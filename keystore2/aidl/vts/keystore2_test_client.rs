// Copyright 2020, The Android Open Source Project
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

#[cfg(test)]
mod tests {

    use std::ops::Deref;

    use android_hardware_keymint::aidl::android::hardware::keymint::{
        Algorithm::Algorithm, Digest::Digest, EcCurve::EcCurve, ErrorCode::ErrorCode,
        KeyPurpose::KeyPurpose, Tag::Tag,
    };
    use android_system_keystore2::aidl::android::system::keystore2::{
        Certificate::Certificate, CertificateChain::CertificateChain, Domain::Domain,
        IKeystoreOperation::IKeystoreOperation, IKeystoreSecurityLevel::IKeystoreSecurityLevel,
        IKeystoreService::IKeystoreService, KeyDescriptor::KeyDescriptor,
        KeyParameter::KeyParameter, OperationChallenge::OperationChallenge,
        ResponseCode::ResponseCode, SecurityLevel::SecurityLevel,
    };
    use anyhow::{anyhow, Result};

    use android_system_keystore2::binder::{ExceptionCode, Result as BinderResult};

    static KS2_SERVICE_NAME: &str = "android.system.keystore2";
    const SELINUX_SHELL_NAMESPACE: i64 = 1;

    fn get_connection() -> Box<dyn IKeystoreService> {
        binder::get_interface(KS2_SERVICE_NAME).unwrap()
    }

    pub fn map_ks_error<T>(r: BinderResult<T>) -> Result<T> {
        r.map_err(|s| {
            match s.exception_code() {
                ExceptionCode::SERVICE_SPECIFIC => {
                    let se = s.service_specific_error();
                    if se < 0 {
                        // Negative service specific errors are KM error codes.
                        anyhow!(format!("Service specific error code {:?}", ErrorCode(se)))
                    } else {
                        // Positive service specific errors are KS response codes.
                        anyhow!(format!("Service specific response code {:?}", ResponseCode(se)))
                    }
                }
                // We create `Error::Binder` to preserve the exception code
                // for logging.
                // `map_or_log_err` will map this on a system error.
                e_code => anyhow!("Binder Exception code {:?}", e_code),
            }
        })
    }

    fn make_ec_signing_key<S: IKeystoreSecurityLevel + ?Sized>(
        sec_level: &S,
        alias: &str,
    ) -> Result<(KeyDescriptor, Option<Certificate>, Option<CertificateChain>)> {
        let gen_params = AuthSetBuilder::new()
            .algorithm(Algorithm::EC)
            .purpose(KeyPurpose::SIGN)
            .purpose(KeyPurpose::VERIFY)
            .digest(Digest::SHA_2_256)
            .ec_curve(EcCurve::P_256);
        let mut key: KeyDescriptor = Default::default();
        let mut cert: Option<Certificate> = None;
        let mut cert_chain: Option<CertificateChain> = None;

        map_ks_error(sec_level.generateKey(
            &KeyDescriptor {
                domain: Domain::SELINUX,
                nspace: SELINUX_SHELL_NAMESPACE,
                alias: Some(alias.to_string()),
                blob: None,
            },
            &gen_params,
            b"entropy",
            &mut key,
            &mut cert,
            &mut cert_chain,
        ))?;

        Ok((key, cert, cert_chain))
    }

    struct AuthSetBuilder(Vec<KeyParameter>);

    impl AuthSetBuilder {
        fn new() -> Self {
            Self(Vec::new())
        }
        fn purpose(mut self, p: KeyPurpose) -> Self {
            self.0.push(KeyParameter { tag: Tag::PURPOSE.0, integer: p.0, ..Default::default() });
            self
        }
        fn digest(mut self, d: Digest) -> Self {
            self.0.push(KeyParameter { tag: Tag::DIGEST.0, integer: d.0, ..Default::default() });
            self
        }
        fn algorithm(mut self, a: Algorithm) -> Self {
            self.0.push(KeyParameter { tag: Tag::ALGORITHM.0, integer: a.0, ..Default::default() });
            self
        }
        fn ec_curve(mut self, e: EcCurve) -> Self {
            self.0.push(KeyParameter { tag: Tag::EC_CURVE.0, integer: e.0, ..Default::default() });
            self
        }
    }

    impl Deref for AuthSetBuilder {
        type Target = Vec<KeyParameter>;

        fn deref(&self) -> &Self::Target {
            &self.0
        }
    }

    #[test]
    fn keystore2_operation_test() -> Result<()> {
        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, "EC_KEY").unwrap();
        assert!(cert.is_none());
        assert!(cert_chain.is_none());

        let mut challenge: Option<OperationChallenge> = None;
        let op = map_ks_error(sec_level.create(
            &key,
            &AuthSetBuilder::new().purpose(KeyPurpose::SIGN).digest(Digest::SHA_2_256),
            false,
            &mut challenge,
        ))
        .unwrap();
        assert!(challenge.is_none());
        assert!(map_ks_error(op.update(b"my message")).unwrap().is_none());
        let signature = map_ks_error(op.finish(None, None)).unwrap();
        assert!(signature.is_some());
        Ok(())
    }

    #[test]
    fn keystore2_backend_busy_test() {
        let keystore2 = get_connection();
        let sec_level =
            map_ks_error(keystore2.getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)).unwrap();
        let (key, cert, cert_chain) = make_ec_signing_key(&*sec_level, "EC_KEY").unwrap();
        assert!(cert.is_none());
        assert!(cert_chain.is_none());

        let mut challenge: Option<OperationChallenge> = None;
        let mut ops: Vec<Box<dyn IKeystoreOperation>> = Vec::new();
        loop {
            match sec_level.create(
                &key,
                &AuthSetBuilder::new().purpose(KeyPurpose::SIGN).digest(Digest::SHA_2_256),
                false,
                &mut challenge,
            ) {
                Err(s) => {
                    assert_eq!(ExceptionCode::SERVICE_SPECIFIC, s.exception_code());
                    assert_eq!(ResponseCode::BACKEND_BUSY.0, s.service_specific_error());
                    println!("BackendBusy reached after {} operation creations.", ops.len());
                    break;
                }
                Ok(op) => {
                    ops.push(op);
                    // If we get to 100 ops without getting to BackendBusy there is something wrong
                    // with the pruning mechanism.
                    assert!(ops.len() < 100);
                    continue;
                }
            }
        }
    }

    #[test]
    fn keystore2_test_client() {
        let keystore2 = get_connection();

        assert_eq!(
            Err((ExceptionCode::SERVICE_SPECIFIC, ResponseCode::SYSTEM_ERROR)),
            keystore2
                .deleteKey(&KeyDescriptor {
                    domain: Domain::APP,
                    nspace: 0,
                    alias: None,
                    blob: None
                })
                .map_err(|s| (s.exception_code(), ResponseCode(s.service_specific_error())))
        );
        if let Err(error) = keystore2
            .getSecurityLevel(SecurityLevel::SOFTWARE)
            .map_err(|s| ErrorCode(s.service_specific_error()))
        {
            assert_eq!(error, ErrorCode::HARDWARE_TYPE_UNAVAILABLE);
        } else {
            panic!("getSecurityLevel with SecurityLevel::SOFTWARE should have failed.");
        }

        if let Ok(sec_level) = keystore2
            .getSecurityLevel(SecurityLevel::TRUSTED_ENVIRONMENT)
            .map_err(|s| s.service_specific_error())
        {
            let gen_params = AuthSetBuilder::new()
                .algorithm(Algorithm::EC)
                .purpose(KeyPurpose::SIGN)
                .purpose(KeyPurpose::VERIFY)
                .digest(Digest::SHA_2_256)
                .ec_curve(EcCurve::P_256);
            let mut key: KeyDescriptor = Default::default();
            let mut cert: Option<Certificate> = None;
            let mut cert_chain: Option<CertificateChain> = None;
            assert_eq!(
                Ok(()),
                sec_level
                    .generateKey(
                        &KeyDescriptor {
                            domain: Domain::SELINUX,
                            nspace: SELINUX_SHELL_NAMESPACE,
                            alias: Some("test_key".to_string()),
                            blob: None
                        },
                        &gen_params,
                        b"entropy",
                        &mut key,
                        &mut cert,
                        &mut cert_chain
                    )
                    .map_err(|s| (s.exception_code(), s.service_specific_error()))
            );
        } else {
            panic!(
                "getSecurityLevel with SecurityLevel::TRUSTED_ENVIRONMENT should have succeeded."
            );
        }
    }
}
