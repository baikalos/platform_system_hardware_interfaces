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

#ifndef ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H
#define ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H

#include <android/system/suspend/WakeLockInfo.h>
#include <utils/Mutex.h>

#include <list>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

using TimestampType = int64_t;

/*
 * WakeLockEntry to collect wake lock stats.
 */
class WakeLockEntry {
   public:
    WakeLockEntry(const std::string& name, int pid, TimestampType epochTimeNow);
    void updateOnAcquire(TimestampType epochTimeNow);
    void updateOnRelease(TimestampType epochTimeNow);
    std::string getName() const;
    int getPid() const;
    WakeLockInfo getWakeLockInfo() const;

   private:
    WakeLockInfo mInfo;
};

/*
 * Hash for WakeLockEntry key (pair<std::string, int>)
 */
struct LockHash {
    std::size_t operator()(const std::pair<std::string, int>& key) const {
        return std::hash<std::string>()(key.first) ^ std::hash<int>()(key.second);
    }
};

/*
 * WakeLockEntryList to collect wake lock stats.
 * This class is thread safe.
 */
class WakeLockEntryList {
   public:
    WakeLockEntryList(int capacity);
    void updateOnAcquire(const std::string& name, int pid, TimestampType epochTimeNow);
    void updateOnRelease(const std::string& name, int pid, TimestampType epochTimeNow);
    void getWakeLockStats(std::vector<WakeLockInfo>* aidl_return) const;

   private:
    int mCapacity;
    mutable std::mutex mStatsLock;
    // std::list and std::unordered map are used to support both inserting a stat
    // and eviction of the LRU stat in O(1) time.
    std::list<WakeLockEntry> mStats GUARDED_BY(mStatsLock);
    std::unordered_map<std::pair<std::string, int>, std::list<WakeLockEntry>::iterator, LockHash>
        mLookupTable GUARDED_BY(mStatsLock);
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H
