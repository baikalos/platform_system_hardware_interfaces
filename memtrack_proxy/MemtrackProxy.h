/*
 * Copyright (C) 2021 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/memtrack/DeviceInfo.h>
#include <aidl/android/hardware/memtrack/IMemtrack.h>
#include <aidl/android/hardware/memtrack/MemtrackRecord.h>
#include <aidl/android/hardware/memtrack/MemtrackType.h>
#include <aidl/android/system/memtrack_proxy/BnMemtrackProxy.h>
#include <android/hardware/memtrack/1.0/IMemtrack.h>

using aidl::android::hardware::memtrack::DeviceInfo;
using aidl::android::hardware::memtrack::MemtrackRecord;
using aidl::android::hardware::memtrack::MemtrackType;
using ::android::sp;

namespace V1_0_hidl = ::android::hardware::memtrack::V1_0;
namespace V_aidl = ::aidl::android::hardware::memtrack;

namespace aidl {
namespace android {
namespace system {
namespace memtrack_proxy {

class MemtrackProxy : public BnMemtrackProxy {
   public:
    MemtrackProxy();
    ndk::ScopedAStatus getMemory(int pid, MemtrackType type,
                                 std::vector<MemtrackRecord>* _aidl_return) override;
    ndk::ScopedAStatus getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) override;

   private:
    static sp<V1_0_hidl::IMemtrack> MemtrackHidlInstance();
    static std::shared_ptr<V_aidl::IMemtrack> MemtrackAidlInstance();
    static bool CheckUid(uid_t calling_uid);
    static bool CheckPid(pid_t calling_pid, pid_t request_pid);

    sp<V1_0_hidl::IMemtrack> memtrack_hidl_instance_;
    std::shared_ptr<V_aidl::IMemtrack> memtrack_aidl_instance_;
};

}  // namespace memtrack_proxy
}  // namespace system
}  // namespace android
}  // namespace aidl
