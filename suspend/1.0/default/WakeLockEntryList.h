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

#ifndef ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H_
#define ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H_

#include "WakeLockEntry.h"

#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

/*
 * WakeLockEntryList to collect wake lock stats.
 * This class is thread safe.
 */
class WakeLockEntryList {
   public:
    WakeLockEntryList() = default;
    void setCapacity(int capacity);
    void updateOnAcquire(std::string name, int pid, long epochTimeNow /* in us */);
    void updateOnRelease(std::string name, long epochTimeNow /* in us */);
    void getWakeLockStats(std::vector<WakeLockInfo>* aidl_return);

   private:
    int capacity;
    std::list<WakeLockEntry> stats;
    std::unordered_map<std::string, std::list<WakeLockEntry>::iterator> lookupTable;
    std::mutex statsLock;
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H_
