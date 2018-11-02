#include <android-base/logging.h>
#include <android/security/keystore/BnKeystoreOperationResultCallback.h>
#include <android/security/keystore/BnKeystoreResponseCallback.h>
#include <android/security/keystore/IKeystoreService.h>
#include <binder/IServiceManager.h>
#include <private/android_filesystem_config.h>

#include <keystore/KeyCharacteristics.h>
#include <keystore/KeymasterArguments.h>
#include <keystore/KeymasterBlob.h>
#include <keystore/KeystoreResponse.h>
#include <keystore/OperationResult.h>
#include <keystore/keymaster_types.h>
#include <keystore/keystore.h>
#include <keystore/keystore_hidl_support.h>
#include <keystore/keystore_return_types.h>

#include <future>
#include <vector>
#include "include/wifikeystorehal/keystore.h"

using android::hardware::keymaster::V4_0::Algorithm;
using android::hardware::keymaster::V4_0::authorizationValue;
using android::hardware::keymaster::V4_0::Digest;
using android::hardware::keymaster::V4_0::KeyFormat;
using android::hardware::keymaster::V4_0::KeyParameter;
using android::hardware::keymaster::V4_0::KeyPurpose;
using android::hardware::keymaster::V4_0::NullOr;
using android::hardware::keymaster::V4_0::PaddingMode;
using android::hardware::keymaster::V4_0::TAG_ALGORITHM;
using android::hardware::keymaster::V4_0::TAG_DIGEST;
using android::hardware::keymaster::V4_0::TAG_PADDING;
using android::security::keymaster::ExportResult;
using android::security::keymaster::KeyCharacteristics;
using android::security::keymaster::KeymasterArguments;
using android::security::keymaster::KeymasterBlob;
using android::security::keymaster::OperationResult;

using KSReturn = keystore::KeyStoreNativeReturnCode;

namespace {
constexpr const char kKeystoreServiceName[] = "android.security.keystore";
constexpr int32_t UID_SELF = -1;

template <typename BnInterface, typename Result>
class CallbackPromise : public BnInterface, public std::promise<Result> {
   public:
    ::android::binder::Status onFinished(const Result& result) override {
        this->set_value(result);
        return ::android::binder::Status::ok();
    }
};

using OperationResultPromise =
    CallbackPromise<::android::security::keystore::BnKeystoreOperationResultCallback,
                    ::android::security::keymaster::OperationResult>;

using KeystoreResponsePromise =
    CallbackPromise<::android::security::keystore::BnKeystoreResponseCallback,
                    ::android::security::keystore::KeystoreResponse>;

};  // namespace

#define AT __func__ << ":" << __LINE__ << " "

namespace android {
namespace system {
namespace wifi {
namespace keystore {
namespace V1_0 {
namespace implementation {

using security::keystore::IKeystoreService;
// Methods from ::android::hardware::wifi::keystore::V1_0::IKeystore follow.
Return<void> Keystore::getBlob(const hidl_string& key, getBlob_cb _hidl_cb) {
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(
        defaultServiceManager()->getService(String16(kKeystoreServiceName)));
    if (service == nullptr) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    ::std::vector<uint8_t> value;
    // Retrieve the blob as wifi user.
    auto ret = service->get(String16(key.c_str()), AID_WIFI, &value);
    if (!ret.isOk()) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    _hidl_cb(KeystoreStatusCode::SUCCESS, (hidl_vec<uint8_t>)value);
    return Void();
}

Return<void> Keystore::getPublicKey(const hidl_string& keyId, getPublicKey_cb _hidl_cb) {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16(kKeystoreServiceName));
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(binder);

    if (service == nullptr) {
        LOG(ERROR) << AT << "could not contact keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    ExportResult result;
    auto binder_result = service->exportKey(
        String16(keyId.c_str()), static_cast<int32_t>(KeyFormat::X509),
        KeymasterBlob() /* clientId */, KeymasterBlob() /* appData */, UID_SELF, &result);
    if (!binder_result.isOk()) {
        LOG(ERROR) << AT << "communication error while calling keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    if (!result.resultCode.isOk()) {
        LOG(ERROR) << AT << "exportKey failed: " << int32_t(result.resultCode);
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    _hidl_cb(KeystoreStatusCode::SUCCESS, result.exportData);
    return Void();
}

static NullOr<const Algorithm&> getKeyAlgoritmFromKeyCharacteristics(
    const ::android::security::keymaster::KeyCharacteristics& characteristics) {
    for (const auto& param : characteristics.hardwareEnforced.getParameters()) {
        auto algo = authorizationValue(TAG_ALGORITHM, param);
        if (algo.isOk()) return algo;
    }
    for (const auto& param : characteristics.softwareEnforced.getParameters()) {
        auto algo = authorizationValue(TAG_ALGORITHM, param);
        if (algo.isOk()) return algo;
    }
    return {};
}

Return<void> Keystore::sign(const hidl_string& keyId, const hidl_vec<uint8_t>& dataToSign,
                            sign_cb _hidl_cb) {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16(kKeystoreServiceName));
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(binder);

    if (service == nullptr) {
        LOG(ERROR) << AT << "could not contact keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    KeyCharacteristics keyCharacteristics;
    String16 key_name16(keyId.c_str());
    int32_t aidl_result;
    auto binder_result = service->getKeyCharacteristics(
        key_name16, KeymasterBlob(), KeymasterBlob(), UID_SELF, &keyCharacteristics, &aidl_result);
    if (!binder_result.isOk()) {
        LOG(ERROR) << AT << "communication error while calling keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    if (KSReturn(aidl_result).isOk()) {
        LOG(ERROR) << AT << "getKeyCharacteristics failed: " << aidl_result;
    }

    auto algorithm = getKeyAlgoritmFromKeyCharacteristics(keyCharacteristics);
    if (!algorithm.isOk()) {
        LOG(ERROR) << AT << "could not get algorithm from key characteristics";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    hidl_vec<KeyParameter> params(3);
    params[0] = Authorization(TAG_DIGEST, Digest::NONE);
    params[1] = Authorization(TAG_PADDING, PaddingMode::NONE);
    params[2] = Authorization(TAG_ALGORITHM, algorithm.value());

    int32_t error_code;
    android::sp<android::IBinder> token(new android::BBinder);
    sp<OperationResultPromise> promise(new OperationResultPromise());
    auto future = promise->get_future();
    binder_result = service->begin(promise, token, key_name16, (int)KeyPurpose::SIGN,
                                   true /*pruneable*/, KeymasterArguments(params),
                                   std::vector<uint8_t>() /* entropy */, UID_SELF, &error_code);
    if (!binder_result.isOk()) {
        LOG(ERROR) << AT << "communication error while calling keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    auto rc = KSReturn(error_code);
    if (!rc.isOk()) {
        LOG(ERROR) << AT << "Keystore begin returned: " << int32_t(rc);
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    OperationResult result = future.get();
    if (!result.resultCode.isOk()) {
        LOG(ERROR) << AT << "begin failed: " << int32_t(result.resultCode);
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    auto handle = std::move(result.token);

    const uint8_t* in = dataToSign.data();
    size_t len = dataToSign.size();
    do {
        future = {};
        binder_result = service->update(promise, handle, KeymasterArguments(params),
                                        std::vector<uint8_t>(in, in + len), &error_code);
        if (!binder_result.isOk()) {
            LOG(ERROR) << AT << "communication error while calling keystore";
            _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
            return Void();
        }

        rc = KSReturn(error_code);
        if (!rc.isOk()) {
            LOG(ERROR) << AT << "Keystore update returned: " << int32_t(rc);
            _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
            return Void();
        }

        result = future.get();

        if (!result.resultCode.isOk()) {
            LOG(ERROR) << AT << "update failed: " << int32_t(result.resultCode);
            _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
            return Void();
        }
        if ((size_t)result.inputConsumed > len) {
            LOG(ERROR) << AT << "update consumed more data than provided";
            sp<KeystoreResponsePromise> abortPromise(new KeystoreResponsePromise);
            auto abortFuture = abortPromise->get_future();
            binder_result = service->abort(abortPromise, handle, &aidl_result);
            if (!binder_result.isOk()) {
                LOG(ERROR) << AT << "communication error while calling keystore";
                _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
                return Void();
            }
            // This is mainly for logging since we already failed.
            // But if abort returned OK we have to wait untill abort calls the callback
            // hence the call to abortFuture.get().
            if (!KSReturn(aidl_result).isOk()) {
                LOG(ERROR) << AT << "abort failed: " << aidl_result;
            } else if (!(rc = KSReturn(abortFuture.get().response_code())).isOk()) {
                LOG(ERROR) << AT << "abort failed: " << int32_t(rc);
            }
            _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
            return Void();
        }
        len -= result.inputConsumed;
        in += result.inputConsumed;
    } while (len > 0);

    future = {};
    promise = new OperationResultPromise();
    future = promise->get_future();

    binder_result = service->finish(promise, handle, KeymasterArguments(params),
                                    std::vector<uint8_t>() /* signature */,
                                    std::vector<uint8_t>() /* entropy */, &error_code);
    if (!binder_result.isOk()) {
        LOG(ERROR) << AT << "communication error while calling keystore";
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    rc = KSReturn(error_code);
    if (!rc.isOk()) {
        LOG(ERROR) << AT << "Keystore finish returned: " << int32_t(rc);
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    result = future.get();

    if (!result.resultCode.isOk()) {
        LOG(ERROR) << AT << "finish failed: " << int32_t(result.resultCode);
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }

    _hidl_cb(KeystoreStatusCode::SUCCESS, result.data);
    return Void();
}

IKeystore* HIDL_FETCH_IKeystore(const char* /* name */) {
    return new Keystore();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace keystore
}  // namespace wifi
}  // namespace system
}  // namespace android
