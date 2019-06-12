/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_H_
#define ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_H_

#include <android/system/suspend/WakeLockInfo.h>

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

/*
 * WakeLockEntry to collect wake lock stats.
 */
class WakeLockEntry {
   public:
    WakeLockEntry(std::string name, int pid, long epochTimeNow /* in us */);

    void updateOnAcquire(long epochTimeNow /* in us */);
    void updateOnRelease(long epochTimeNow /* in us */);
    std::string getName();
    WakeLockInfo getWakeLockInfo() const;

   private:
    std::string name;
    int pid;
    long activeSince = 0;
    long activeCount = 0;
    long lastChange = 0;
    long maxTime = 0;
    long totalTime = 0;
    bool isActive = false;

    // Kernel Wakelock stats
    long eventCount = 0;
    long wakeupCount = 0;
    long expireCount = 0;
    long preventSuspendTime = 0;
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_H_
