/*
 * Copyright 2019 The Android Open Source Project
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

#include "SuspendControlService.h"

#include <android-base/file.h>
#include <android-base/logging.h>

#include "SystemSuspend.h"

using ::android::base::ReadFdToString;

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

template <typename T>
binder::Status retOk(const T& value, T* ret_val) {
    *ret_val = value;
    return binder::Status::ok();
}

void SuspendControlService::setSuspendService(const wp<SystemSuspend>& suspend) {
    mSuspend = suspend;
}

binder::Status SuspendControlService::enableAutosuspend(bool* _aidl_return) {
    const auto suspendService = mSuspend.promote();
    return retOk(suspendService != nullptr && suspendService->enableAutosuspend(), _aidl_return);
}

binder::Status SuspendControlService::registerCallback(const sp<ISuspendCallback>& callback,
                                                       bool* _aidl_return) {
    if (!callback) {
        return retOk(false, _aidl_return);
    }

    auto l = std::lock_guard(mCallbackLock);
    sp<IBinder> cb = IInterface::asBinder(callback);
    // Only remote binders can be linked to death
    if (cb->remoteBinder() != nullptr) {
        if (findCb(cb) == mCallbacks.end()) {
            auto status = cb->linkToDeath(this);
            if (status != NO_ERROR) {
                LOG(ERROR) << __func__ << " Cannot link to death: " << status;
                return retOk(false, _aidl_return);
            }
        }
    }
    mCallbacks.push_back(callback);
    return retOk(true, _aidl_return);
}

binder::Status SuspendControlService::forceSuspend(bool* _aidl_return) {
    const auto suspendService = mSuspend.promote();
    return retOk(suspendService != nullptr && suspendService->forceSuspend(), _aidl_return);
}

void SuspendControlService::binderDied(const wp<IBinder>& who) {
    auto l = std::lock_guard(mCallbackLock);
    mCallbacks.erase(findCb(who));
}

void SuspendControlService::notifyWakeup(bool success) {
    // A callback could potentially modify mCallbacks (e.g., via registerCallback). That must not
    // result in a deadlock. To that end, we make a copy of mCallbacks and release mCallbackLock
    // before calling the copied callbacks.
    auto callbackLock = std::unique_lock(mCallbackLock);
    auto callbacksCopy = mCallbacks;
    callbackLock.unlock();

    for (const auto& callback : callbacksCopy) {
        callback->notifyWakeup(success).isOk();  // ignore errors
    }
}

binder::Status SuspendControlService::getWakeLockStats(std::vector<WakeLockInfo>* _aidl_return) {
    const auto suspendService = mSuspend.promote();
    if (!suspendService) {
        return binder::Status::fromExceptionCode(binder::Status::Exception::EX_NULL_POINTER,
                                                 String8("Null reference to suspendService"));
    }

    suspendService->updateStatsNow();
    suspendService->getStatsList().getWakeLockStats(_aidl_return);

    return binder::Status::ok();
}

binder::Status SuspendControlService::getStackTraces(std::string* _aidl_return) {
    const auto suspendService = mSuspend.promote();

    if (!suspendService) {
        return binder::Status::fromExceptionCode(binder::Status::Exception::EX_NULL_POINTER,
                                                 String8("Null reference to suspendService"));
    }

    // Only traces
    std::vector<hidl_string> options;
    options.emplace_back("--traces");

    // Index 0 corresponds to the read end of the pipe; 1 to the write end.
    int fds[2];
    pipe2(fds, O_NONBLOCK);
    native_handle_t* handle = native_handle_create(1, 0);
    handle->data[0] = fds[1];

    suspendService->debug(handle, options);

    ReadFdToString(fds[0], _aidl_return);

    native_handle_delete(handle);
    close(fds[0]);
    close(fds[1]);

    return binder::Status::ok();
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
