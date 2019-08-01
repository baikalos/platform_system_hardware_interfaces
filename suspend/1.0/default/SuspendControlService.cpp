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

#include <android-base/logging.h>

#include "SystemSuspend.h"

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

static std::string dumpUsage() {
    return "\nUsage: adb shell dumpsys suspend_control [options...]\n\n"
           "   Options:\n"
           "       --wakelocks      : returns wakelock stats.\n"
           "       --suspend_stats  : returns suspend stats.\n"
           "       --all            : returns all stats. Same as providing no stat option.\n"
           "       --help or -h     : prints this message, regardless of any other option(s)\n"
           "                          provided.\n\n";
}

status_t SuspendControlService::dump(int fd, const Vector<String16>& args) {
    const auto suspendService = mSuspend.promote();
    if (!suspendService) {
        return DEAD_OBJECT;
    }

    bool all = true;
    bool wakelocks = false;
    bool suspend_stats = false;
    bool help = false;

    // Parse args
    for (const String16& a : args) {
        std::string arg(String8(a).string());
        if (arg == "--wakelocks") {
            wakelocks = true;
            all = false;
        } else if (arg == "--suspend_stats") {
            suspend_stats = true;
            all = false;
        } else if (arg == "--all") {
            all = true;
        } else if (arg == "-h" || arg == "--help") {
            help = true;
            break;
        }
    }

    if (help) {
        std::string usage = dumpUsage();
        dprintf(fd, "%s\n", usage.c_str());
    } else {
        if (wakelocks || all) {
            suspendService->updateStatsNow();
            std::stringstream wlStats;
            wlStats << suspendService->getStatsList();

            dprintf(fd, "\n%s\n", wlStats.str().c_str());
        }

        if (suspend_stats || all) {
            std::string ss = suspendService->getSuspendStats();
            dprintf(fd, "\n%s\n", ss.c_str());
        }
    }

    return OK;
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
