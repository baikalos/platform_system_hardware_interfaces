/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system.keystore2;

import android.system.keystore2.AuthenticatorSpec;
import android.system.keystore2.Certificate;
import android.system.keystore2.CertificateChain;
import android.system.keystore2.IKeystoreOperation;
import android.system.keystore2.KeyDescriptor;
import android.system.keystore2.KeyParameter;
import android.system.keystore2.OperationChallenge;

/**
 * `IKeystoreSecurityLevel` is the per backend interface to Keystore. It provides
 * access to all requests that require KeyMint interaction, such as key import
 * and generation, as well as cryptographic operations.
 *
 * ## Error conditions
 * Error conditions are reported as service specific error.
 * Positive codes correspond to `android.system.keystore2.ResponseCode`
 * and indicate error conditions diagnosed by the Keystore 2.0 service.
 * Negative codes correspond to `android.hardware.keymint.ErrorCode` and
 * indicate KeyMint back end errors. Refer to the KeyMint interface spec for
 * detail.
 */
@VintfStability
interface IKeystoreSecurityLevel {

    /**
     * This function creates a new key operation. Operations are the mechanism by which the
     * secret or private key material of a key can be used. There is a limited number
     * of operation slots. Implementations may prune an existing operation to make room
     * for a new one. The pruning strategy is implementation defined, but it must
     * account for forced operations (see parameter `forced` below).
     * Forced operations require the caller to possess the `REQ_FORCED_OP` permission.
     *
     * ## Pruning strategy recommendation
     * It is recommended to choose a strategy that rewards "good" behavior.
     * It is considered good behavior not to hog operations. Clients that use
     * few parallel operations shall have a better chance of starting and finishing
     * an operations than those that use many. Clients that use frequently update their
     * operations shall have a better chance to complete them successfully that those
     * that let their operations linger.
     *
     * ## Error conditions
     * `ResponseCode::BACKEND_BUSY` if the implementation was unable to find a free
     *            or free up an operation slot for the new operation.
     *
     * @param key Describes the key that is to be used for the operation.
     *
     * @param operationParameters Additional operation parameters that describe the nature
     *            of the requested operation.
     *
     * @param forced A forced operation has a very high pruning power. The implementation may
     *            select an operation to be pruned that would not have been pruned otherwise to
     *            free up an operation slot for the caller. Also, the resulting operation shall
     *            have a very high pruning resistance and cannot be pruned even by other forced
     *            operations.
     *
     * @param challenge Information required for acquiring user authorization for the started
     *            operation. This field is nullable/optional. If present, user authorization is
     *            required.
     *
     * @return The operation interface which also acts as a handle to the pending
     *            operation.
     */
    IKeystoreOperation create(in KeyDescriptor key, in KeyParameter[] operationParameters,
                  in boolean forced, out @nullable OperationChallenge challenge);

    /**
     * Generates a new key and associates it with the given descriptor.
     *
     * @param key The domain field of the key descriptor governs how the key will be stored.
     *            * App: The key is stored by the given alias string in the implicit UID namespace
     *                   of the caller.
     *            * SeLinux: The key is stored by the alias string in the namespace given by the
     *                       `nspace` field provided the caller has the appropriate access rights.
     *            * Blob: The key is returned as raw keymint blob in the resultKey.blob field.
     *                    The `nspace` and `alias` fields are ignored. The caller must have the
     *                    `MANAGE_BLOB` permission for the keystore:keystore_key context.
     *
     * ## Error conditions
     * `ResponseCode::INVALID_ARGUMENT` if `key.domain` is set to any other value than
     *                   the ones described above.
     * `ResponseCode::KEY_NOT_FOUND` if the specified key did not exist.
     * A KeyMint ResponseCode may be returned indicating a backend diagnosed error.
     *
     * @param params Describes the characteristics of the to be generated key. See KeyMint HAL
     *                   for details.
     *
     * @param entropy This array of random bytes is mixed into the entropy source used for key
     *                   generation.
     *
     * @param resultKey A key descriptor that can be used for subsequent key operations.
     *                   If `Domain::BLOB` was requested, then the descriptor contains the
     *                   generated key, and the caller must assure that the key is persistently
     *                   stored accordingly; there is no way to recover the key if the blob is
     *                   lost.
     *
     * @param publicCert The generated public certificate if applicable. If `Domain::BLOB` was
     *                   requested, there is no other copy of this certificate. It is the caller's
     *                   responsibility to store it persistently if required.
     *
     * @param certificateChain The generated certificate chain if applicable. If `Domain::BLOB` was
     *                   requested, there is no other copy of this certificate chain. It is the
     *                   caller's responsibility to store it persistently if required.
     */
    void generateKey(in KeyDescriptor key, in KeyParameter[] params, in byte[] entropy,
                     out KeyDescriptor resultKey, out @nullable Certificate publicCert,
                     out @nullable CertificateChain certificateChain);


    /**
     * Imports the given key. This API call works exactly like `generateKey`, only that the key is
     * provided by the caller rather than being generated by Keymint. We only describe
     * the parameters where they deviate from the ones of `generateKey`.
     *
     * @param keyData The key to be imported. Expected encoding is PKCS#8 for asymmetric keys and
     *                raw key bits for symmetric keys.
     */
    void importKey(in KeyDescriptor key, in KeyParameter[] params, in byte[] keyData,
                   out KeyDescriptor resultKey, out @nullable Certificate publicCert,
                   out @nullable CertificateChain certificateChain);

    /**
     * Allows importing keys wrapped with an RSA encryption key that is stored in AndroidKeystore.
     *
     * @param key Governs how the imported key shall be stored. See `generateKey` for details.
     *
     * @param wrappingKey Indicates the key that shall be used for unwrapping the wrapped key
     *                    in a manner similar to starting a new operation with create.
     *
     * @param maskingKey Reserved for future use. Must be null for now.
     *
     * @param params These parameters describe the cryptographic operation that shall be performed
     *               using the wrapping key in order to unwrap the wrapped key.
     *
     * @param authenticators When generating or importing a key that is bound to a specific
     *                       authenticator, the authenticator ID is included in the key parameters.
     *                       Imported wrapped keys can also be authentication bound, however, the
     *                       key parameters were included in the wrapped key at a remote location
     *                       where the device's authenticator ID is not known. Therefore, the
     *                       caller has to provide all of the possible authenticator IDs so that
     *                       Keymint can pick the right one based on the included key parameters.
     *
     * @return resultKey, publicCert, and certificateChain see `generateKey`.
     */
    void importWrappedKey(in KeyDescriptor key, in KeyDescriptor wrappingKey,
                          in @nullable byte[] maskingKey, in KeyParameter[] params,
                          in AuthenticatorSpec[] authenticators,
                          out KeyDescriptor resultKey, out @nullable Certificate publicCert,
                          out @nullable CertificateChain certificateChain);
}