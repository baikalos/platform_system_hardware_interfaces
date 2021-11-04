// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#![allow(dead_code)]

use android_hardware_security_keymint::aidl::android::hardware::security::keymint::{
    Algorithm::Algorithm,
    Digest::Digest,
    EcCurve::EcCurve, // ErrorCode::ErrorCode,
    KeyParameter::KeyParameter,
    KeyParameterValue::KeyParameterValue,
    KeyPurpose::KeyPurpose,
    SecurityLevel::SecurityLevel,
    Tag::Tag,
};
use lazy_static::lazy_static;
use std::ops::Deref;
use std::{
    collections::HashMap,
    sync::{Condvar, Mutex},
};

use keystore2_test_utils::run_as;

lazy_static! {
    static ref OP_LIMIT: OperationLimiter = OperationLimiter::new();
}

#[derive(Debug, Default)]
struct OperationLimiter {
    max_ops: HashMap<SecurityLevel, u32>,
    current_ops: Mutex<HashMap<SecurityLevel, u32>>,
    cond_var: Condvar,
}

impl OperationLimiter {
    fn new() -> Self {
        let mut max_ops: HashMap<SecurityLevel, u32> = Default::default();
        max_ops.insert(SecurityLevel::TRUSTED_ENVIRONMENT, 15);
        max_ops.insert(SecurityLevel::STRONGBOX, 3);
        Self {
            max_ops,
            ..Default::default()
        }
    }

    fn get(&self, sec_level: SecurityLevel) -> OperationGuard {
        let max_ops = self
            .max_ops
            .get(&sec_level)
            .expect("Requested operation guard for unknown sec level.");
        let mut locked_current_ops = self
            .cond_var
            .wait_while(self.current_ops.lock().unwrap(), |ops| {
                *ops.get(&sec_level).unwrap_or(&0) == *max_ops
            })
            .unwrap();
        *locked_current_ops.entry(sec_level).or_insert(0) += 1;
        OperationGuard(sec_level, 1)
    }

    fn get_exclusive(&self, sec_level: SecurityLevel) -> OperationGuard {
        let max_ops = self
            .max_ops
            .get(&sec_level)
            .expect("Requested operation guard for unknown sec level.");
        let mut locked_current_ops = self
            .cond_var
            .wait_while(self.current_ops.lock().unwrap(), |ops| {
                *ops.get(&sec_level).unwrap_or(&0) != 0u32
            })
            .unwrap();
        *locked_current_ops.entry(sec_level).or_insert(0) += max_ops;
        OperationGuard(sec_level, *max_ops)
    }
}

struct OperationGuard(SecurityLevel, u32);

impl Drop for OperationGuard {
    fn drop(&mut self) {
        let mut locked_current_ops = OP_LIMIT.current_ops.lock().unwrap();
        if locked_current_ops[&self.0] < self.1 {
            panic!("current_ops cannot be less than the number of ops we are freeing.");
        }
        locked_current_ops
            .entry(self.0)
            .and_modify(|e| *e -= self.1);
        drop(locked_current_ops);
        OP_LIMIT.cond_var.notify_all();
    }
}

struct AuthSetBuilder(Vec<KeyParameter>);

impl AuthSetBuilder {
    fn new() -> Self {
        Self(Vec::new())
    }
    fn purpose(mut self, p: KeyPurpose) -> Self {
        self.0.push(KeyParameter {
            tag: Tag::PURPOSE,
            value: KeyParameterValue::KeyPurpose(p),
        });
        self
    }
    fn digest(mut self, d: Digest) -> Self {
        self.0.push(KeyParameter {
            tag: Tag::DIGEST,
            value: KeyParameterValue::Digest(d),
        });
        self
    }
    fn algorithm(mut self, a: Algorithm) -> Self {
        self.0.push(KeyParameter {
            tag: Tag::ALGORITHM,
            value: KeyParameterValue::Algorithm(a),
       });
        self
    }
    fn ec_curve(mut self, e: EcCurve) -> Self {
        self.0.push(KeyParameter {
            tag: Tag::EC_CURVE,
            value: KeyParameterValue::EcCurve(e),
        });
        self
    }
    fn no_auth_required(mut self, b: bool) -> Self {
        self.0.push(KeyParameter {
            tag: Tag::NO_AUTH_REQUIRED,
            value: KeyParameterValue::BoolValue(b),
        });
        self
    }
}

impl Deref for AuthSetBuilder {
    type Target = Vec<KeyParameter>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[cfg(test)]
mod tests {

    use super::*;
    use keystore2_selinux as selinux;
    use nix::unistd::{getgid, getuid, Gid, Uid};

    use android_hardware_security_keymint::aidl::android::hardware::security::keymint::{
        Algorithm::Algorithm,
        Digest::Digest,
        EcCurve::EcCurve,
        ErrorCode::ErrorCode,
        KeyPurpose::KeyPurpose, // Tag::Tag, KeyParameter::KeyParameter,
        SecurityLevel::SecurityLevel,
    };
    use android_system_keystore2::aidl::android::system::keystore2::{
        CreateOperationResponse::CreateOperationResponse, Domain::Domain,
        IKeystoreSecurityLevel::IKeystoreSecurityLevel, IKeystoreService::IKeystoreService,
        KeyDescriptor::KeyDescriptor, ResponseCode::ResponseCode,
    };
    use anyhow::{anyhow, Result};
    use std::convert::TryFrom;
    use std::thread::sleep;
    use std::time::Duration;

    use android_system_keystore2::binder::{ExceptionCode, Result as BinderResult};

    static KS2_SERVICE_NAME: &str = "android.system.keystore2.IKeystoreService/default";
    const SELINUX_SHELL_NAMESPACE: i64 = 1;

    fn get_connection() -> binder::Strong<dyn IKeystoreService> {
        binder::get_interface(KS2_SERVICE_NAME).unwrap()
    }

    #[derive(thiserror::Error, Debug, Eq, PartialEq)]
    enum Error {
        #[error("Responsecode {0:?}")]
        Rc(ResponseCode),
        #[error("ErrorCode {0:?}")]
        Km(ErrorCode),
        #[error("Binder exception {0:?}")]
        Binder(ExceptionCode),
    }

    fn map_ks_error<T>(r: BinderResult<T>) -> Result<T, Error> {
        r.map_err(|s| {
            match s.exception_code() {
                ExceptionCode::SERVICE_SPECIFIC => {
                    match s.service_specific_error() {
                        se if se < 0 => {
                            // Negative service specific errors are KM error codes.
                            Error::Km(ErrorCode(se))
                        }
                        se => {
                            // Positive service specific errors are KS response codes.
                            Error::Rc(ResponseCode(se))
                        }
                    }
                }
                // We create `Error::Binder` to preserve the exception code
                // for logging.
                // `map_or_log_err` will map this on a system error.
                e_code => Error::Binder(e_code),
            }
        })
    }

    fn make_ec_signing_key<S: IKeystoreSecurityLevel + ?Sized>(
        sec_level: &S,
        alias: &str,
    ) -> Result<(KeyDescriptor, Option<Vec<u8>>, Option<Vec<u8>>)> {
        let gen_params = AuthSetBuilder::new()
            .no_auth_required(true)
            .algorithm(Algorithm::EC)
            .purpose(KeyPurpose::SIGN)
            .purpose(KeyPurpose::VERIFY)
            .digest(Digest::SHA_2_256)
            .ec_curve(EcCurve::P_256);

        let key_metadata = map_ks_error(sec_level.generateKey(
            &KeyDescriptor {
                domain: Domain::SELINUX,
                nspace: SELINUX_SHELL_NAMESPACE,
                alias: Some(alias.to_string()),
                blob: None,
            },
            None,
            &gen_params,
            0,
            b"entropy",
        ))?;

        Ok((
            key_metadata.key,
            key_metadata.certificate,
            key_metadata.certificateChain,
        ))
    }

    #[test]
    fn keystore2_runas_pruning_test() {
        let _exclusive_ops = OP_LIMIT.get_exclusive(SecurityLevel::TRUSTED_ENVIRONMENT);

        let mut loop_count = 0;
        let mut id = 10020;
        loop {
            run_as::run_as_app(selinux::getcon().unwrap().to_str().unwrap(), Uid::from_raw(id),
                                Gid::from_raw(id), || {
                let keystore2 = get_connection();
                let sec_level =
                    map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT))
                    .unwrap();
                let alias = format!("{}{}", "keystore2_pruning_test__key", getuid_fromc());
                let (key, cert, cert_chain) =
                    make_ec_signing_key(&*sec_level, &alias).unwrap();
                assert!(cert.is_some());
                assert!(cert_chain.is_none());

                println!("OPERATION :  getuid {} , getgid {}",  getuid(), getgid());
                let mut ops: Vec<CreateOperationResponse> = Vec::new();

                let first_op = map_ks_error(
                    sec_level.createOperation(
                        &key,
                        &AuthSetBuilder::new()
                        .purpose(KeyPurpose::SIGN)
                        .digest(Digest::SHA_2_256),
                        false,
                        ),
                        )
                    .unwrap();

                let result = sec_level.createOperation(
                    &key,
                    &AuthSetBuilder::new()
                    .purpose(KeyPurpose::SIGN)
                    .digest(Digest::SHA_2_256),
                    false,
                    );

                match result {
                    Err(s) => {
                        assert_eq!(ExceptionCode::SERVICE_SPECIFIC, s.exception_code());
                        assert_eq!(ResponseCode::BACKEND_BUSY.0, s.service_specific_error());
                        println!(
                            "BackendBusy reached after {} operation creations.",
                            ops.len()
                            );
                    }
                    Ok(op) => {
                        ops.push(op);
                    }
                }
                // sleep for few secs to let other operations slots get filled.
                sleep(Duration::from_secs(5));

                if let CreateOperationResponse {
                    iOperation: Some(op),
                    operationChallenge: challenge,
                    parameters: _,
                    ..
                } = first_op
                {
                    assert!(challenge.is_none());
                    assert_eq!(
                        Err(Error::Km(ErrorCode::INVALID_OPERATION_HANDLE)),
                        map_ks_error(op.update(b"my message"))
                        );
                }
            });
            loop_count += 1;
            id += 1;
            if loop_count == 18 {
                break;
            }
        }
    }

    #[test]
    fn keystore2_operation_test() -> Result<()> {
        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, "EC_KEY").unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        let _op_guard = OP_LIMIT.get(SecurityLevel::TRUSTED_ENVIRONMENT);

        let op = map_ks_error(
            sec_level.createOperation(
                &key,
                &AuthSetBuilder::new()
                    .purpose(KeyPurpose::SIGN)
                    .digest(Digest::SHA_2_256),
                false,
            ),
        )
        .unwrap();
        if let CreateOperationResponse {
            iOperation: Some(op),
            operationChallenge: challenge,
            parameters: _,
            ..
        } = op
        {
            assert!(challenge.is_none());
            assert!(map_ks_error(op.update(b"my message")).unwrap().is_none());
            let signature = map_ks_error(op.finish(None, None)).unwrap();
            assert!(signature.is_some());
        } else {
            return Err(anyhow!("fail"));
        }
        Ok(())
    }

    #[test]
    fn keystore2_pruning_test() {
        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) =
            make_ec_signing_key(&*sec_level, "keystore2_pruning_test_key").unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        let _exclusive_ops = OP_LIMIT.get_exclusive(SecurityLevel::TRUSTED_ENVIRONMENT);

        let first_op = map_ks_error(
            sec_level.createOperation(
                &key,
                &AuthSetBuilder::new()
                    .purpose(KeyPurpose::SIGN)
                    .digest(Digest::SHA_2_256),
                false,
            ),
        )
        .unwrap();

        let mut ops: Vec<CreateOperationResponse> = Vec::new();
        loop {
            match sec_level.createOperation(
                &key,
                &AuthSetBuilder::new()
                    .purpose(KeyPurpose::SIGN)
                    .digest(Digest::SHA_2_256),
                false,
            ) {
                Err(s) => {
                    assert_eq!(ExceptionCode::SERVICE_SPECIFIC, s.exception_code());
                    assert_eq!(ResponseCode::BACKEND_BUSY.0, s.service_specific_error());
                    println!(
                        "BackendBusy reached after {} operation creations.",
                        ops.len()
                    );
                    break;
                }
                Ok(op) => {
                    ops.push(op);
                    if ops.len() == 100 {
                        break;
                    }
                    continue;
                }
            }
        }
        if let CreateOperationResponse {
            iOperation: Some(op),
            operationChallenge: challenge,
            parameters: _,
            ..
        } = first_op
        {
            assert!(challenge.is_none());
            assert_eq!(
                Err(Error::Km(ErrorCode::INVALID_OPERATION_HANDLE)),
                map_ks_error(op.update(b"my message"))
            );
        }
    }

    #[test]
    fn get_security_level_success() {
        let keystore2 = get_connection();
        assert!(
            keystore2
                .getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)
                .is_ok(),
            "getSecurityLevel with SecurityLevel::TRUSTED_ENVIRONMENT should have succeeded."
        );
    }

    #[test]
    fn get_security_level_failure() {
        let keystore2 = get_connection();
        if let Err(error) = keystore2
            .getSecurityLevel(SecurityLevel::SOFTWARE)
            .map_err(|s| (s.exception_code(), ErrorCode(s.service_specific_error())))
        {
            assert_eq!(
                (
                    ExceptionCode::SERVICE_SPECIFIC,
                    ErrorCode::HARDWARE_TYPE_UNAVAILABLE,
                ),
                error
            );
        } else {
            panic!("getSecurityLevel with SecurityLevel::SOFTWARE should have failed.");
        }
    }

    #[test]
    fn get_key_entry_success() {
        let test_alias = "get_key_entry_success_key";

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Ok(key_entry_response) = keystore2.getKeyEntry(&key) {
            assert_eq!(key, key_entry_response.metadata.key);
            assert_eq!(cert, key_entry_response.metadata.certificate);
            assert_eq!(cert_chain, key_entry_response.metadata.certificateChain);
        } else {
            panic!("getKeyEntry should have succeeded.")
        }
    }

    #[test]
    fn get_key_entry_failure() {
        let test_alias = "get_key_entry_failure_key";

        let keystore2 = get_connection();
        if let Err(error) = keystore2
            .getKeyEntry(&KeyDescriptor {
                domain: Domain::SELINUX,
                nspace: SELINUX_SHELL_NAMESPACE,
                alias: Some(test_alias.to_string()),
                blob: None,
            })
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("getKeyEntry should have failed for non-exsting key.")
        }
    }

    #[test]
    fn update_subcomponent_success() {
        let test_alias = "update_subcomponent_success_key";

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        let other_cert: [u8; 32] = [123; 32];
        let other_cert_chain: [u8; 32] = [12; 32];

        if let Err(_error) =
            keystore2.updateSubcomponent(&key, Some(&other_cert), Some(&other_cert_chain))
        {
            panic!("updateSubcomponent should have succeeded.")
        }

        if let Ok(key_entry_response) = keystore2.getKeyEntry(&key) {
            assert_eq!(
                Some(other_cert.to_vec()),
                key_entry_response.metadata.certificate
            );
            assert_eq!(
                Some(other_cert_chain.to_vec()),
                key_entry_response.metadata.certificateChain
            );
        } else {
            panic!("getKeyEntry should have succeeded.")
        }
    }

    #[test]
    fn update_subcomponent_failure() {
        let test_alias = "update_component_failure_key";

        let keystore2 = get_connection();

        let other_cert: [u8; 32] = [123; 32];
        let other_cert_chain: [u8; 32] = [12; 32];

        if let Err(error) = keystore2
            .updateSubcomponent(
                &KeyDescriptor {
                    domain: Domain::SELINUX,
                    nspace: SELINUX_SHELL_NAMESPACE,
                    alias: Some(test_alias.to_string()),
                    blob: None,
                },
                Some(&other_cert),
                Some(&other_cert_chain),
            )
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("getKeyEntry should have failed for non-exsting key.")
        }
    }

    #[test]
    fn list_entries_success() {
        let test_alias = "list_entries_success_key";

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (_key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Ok(key_descriptors) = keystore2.listEntries(Domain::SELINUX, SELINUX_SHELL_NAMESPACE)
        {
            let mut key_exists = false;
            for key_descriptor in key_descriptors {
                if let Some(alias) = key_descriptor.alias {
                    if alias == test_alias {
                        key_exists = true;
                    }
                }
            }
            assert!(key_exists, "listEntries should have returned test key.");
        } else {
            panic!("listEntries should have succeeded.");
        }
    }

    #[test]
    fn delete_key_success() {
        let test_alias = "delete_key_success_key";

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Err(_error) = keystore2.deleteKey(&key) {
            panic!("deleteKey should have succeeded")
        }

        // Key should already be deleted.
        if let Err(error) = keystore2
            .getKeyEntry(&key)
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("getKeyEntry should have failed for deleted key.")
        }
    }

    fn delete_key_failure() {
        let test_alias = "delete_key_failure_key";

        let keystore2 = get_connection();

        if let Err(error) = keystore2
            .deleteKey(&KeyDescriptor {
                domain: Domain::SELINUX,
                nspace: SELINUX_SHELL_NAMESPACE,
                alias: Some(test_alias.to_string()),
                blob: None,
            })
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("deleteKey should have failed for non-exsting key.")
        }
    }

    fn getuid_fromc() -> i32 {
        unsafe { i32::try_from(libc::getuid()).unwrap() }
    }

    #[test]
    fn grant_success() {
        let test_alias = "grant_success_key";
        let test_uid = getuid_fromc();
        let test_access_vector = 0;

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        // TODO: Uncomment after fixing test
        // if let Ok(key_descriptor) = keystore2.grant(&key, test_uid, test_access_vector) {
        //     assert_eq!(key, key_descriptor);
        // } else {
        //     panic!("grant should have succeeded.")
        // }

        // TODO: Remove after fixing test
        let _result = match keystore2.grant(&key, test_uid, test_access_vector) {
            Ok(result) => result,
            Err(error) => {
                panic!("Error - {}", error);
            }
        };
        // TODO: Remove END
    }

    #[test]
    fn grant_permission_failure() {
        let test_alias = "grant_permission_failure_key";
        let test_uid = 0;
        let test_access_vector = 0;

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Err(error) = keystore2
            .grant(&key, test_uid, test_access_vector)
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (
                    ExceptionCode::SERVICE_SPECIFIC,
                    ResponseCode::PERMISSION_DENIED
                ),
                error
            );
        } else {
            panic!("grant should have failed for uid 0.")
        }
    }

    #[test]
    fn grant_missing_key_failure() {
        let test_alias = "grant_missing_key_failure_key";
        let test_uid = getuid_fromc();
        let test_access_vector = 0;

        let keystore2 = get_connection();

        if let Err(error) = keystore2
            .grant(
                &KeyDescriptor {
                    domain: Domain::SELINUX,
                    nspace: SELINUX_SHELL_NAMESPACE,
                    alias: Some(test_alias.to_string()),
                    blob: None,
                },
                test_uid,
                test_access_vector,
            )
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("grant should have failed for non-exsting key.")
        }
    }

    // TODO: Uncomment after fixing test
    // #[test]
    fn ungrant_success() {
        let test_alias = "ungrant_success_key";
        let test_uid = getuid_fromc();
        let test_access_vector = 0;

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        let granted_key =
            map_ks_error(keystore2.grant(&key, test_uid, test_access_vector)).unwrap();

        if let Err(_error) = keystore2.ungrant(&granted_key, test_uid) {
            panic!("ungrant should have succeeded")
        }
    }

    // TODO: Uncomment after fixing test
    // #[test]
    fn ungrant_no_key_failure() {
        let test_alias = "ungrant_no_key_failure_key";
        let test_uid = getuid_fromc();

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Err(error) = keystore2
            .ungrant(&key, test_uid)
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (ExceptionCode::SERVICE_SPECIFIC, ResponseCode::KEY_NOT_FOUND),
                error
            );
        } else {
            panic!("ungrant should have failed for non-existing key.")
        }
    }

    #[test]
    fn ungrant_no_grant_failure() {
        let test_alias = "ungrant_no_grant_failure_key";
        let test_uid = getuid_fromc();

        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, test_alias).unwrap();
        assert!(cert.is_some());
        assert!(cert_chain.is_none());

        if let Err(error) = keystore2
            .ungrant(&key, test_uid)
            .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        {
            assert_eq!(
                (
                    ExceptionCode::SERVICE_SPECIFIC,
                    ResponseCode::PERMISSION_DENIED
                ),
                error
            );
        } else {
            panic!("ungrant should have failed for non-existing grant.")
        }
    }
}
