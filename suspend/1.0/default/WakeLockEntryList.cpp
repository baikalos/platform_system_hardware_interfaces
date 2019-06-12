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

#include "WakeLockEntryList.h"

#include <android-base/logging.h>

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {
/*
WakeLockEntry::WakeLockEntry(const std::string& name, int pid, TimestampType epochTimeNow)
    : mName(name), mPid(pid) {
*/
WakeLockEntry::WakeLockEntry(const std::string& name, int pid, TimestampType epochTimeNow) {
    mInfo.name = name;
    mInfo.pid = pid;
    mInfo.activeCount = 0;
    mInfo.maxTime = 0;
    mInfo.totalTime = 0;

    updateOnAcquire(epochTimeNow);
}

std::string WakeLockEntry::getName() const {
    return mInfo.name;
}

int WakeLockEntry::getPid() const {
    return mInfo.pid;
}

void WakeLockEntry::updateOnAcquire(TimestampType epochTimeNow) {
    mInfo.isActive = true;
    mInfo.activeSince = epochTimeNow;
    mInfo.activeCount++;
    mInfo.lastChange = epochTimeNow;
}

void WakeLockEntry::updateOnRelease(TimestampType epochTimeNow) {
    mInfo.isActive = false;
    mInfo.maxTime = std::max(mInfo.maxTime, epochTimeNow - mInfo.activeSince);
    mInfo.totalTime += epochTimeNow - mInfo.activeSince;
    mInfo.lastChange = epochTimeNow;
}

WakeLockInfo WakeLockEntry::getWakeLockInfo() const {
    return mInfo;
}

WakeLockEntryList::WakeLockEntryList(int capacity) : mCapacity(capacity) {}

void WakeLockEntryList::updateOnAcquire(const std::string& name, int pid,
                                        TimestampType epochTimeNow) {
    std::lock_guard<std::mutex> lock(mStatsLock);

    auto key = std::make_pair(name, pid);
    auto it = mLookupTable.find(key);
    if (it == mLookupTable.end()) {
        // Evict LRU if stats is at capacity
        if (mStats.size() == mCapacity) {
            auto evictIt = mStats.end();
            std::advance(evictIt, -1);
            auto evictKey = std::make_pair(evictIt->getName(), evictIt->getPid());
            mLookupTable.erase(evictKey);
            mStats.erase(evictIt);
        }
        // Insert new entry as MRU
        mStats.push_front(WakeLockEntry(name, pid, epochTimeNow));
        mLookupTable[key] = mStats.begin();
    } else {
        // Update existing entry
        WakeLockEntry updatedEntry = *(it->second);
        updatedEntry.updateOnAcquire(epochTimeNow);

        // Make updated entry MRU
        mStats.erase(it->second);
        mStats.push_front(updatedEntry);
        mLookupTable[key] = mStats.begin();
    }
}

void WakeLockEntryList::updateOnRelease(const std::string& name, int pid,
                                        TimestampType epochTimeNow) {
    std::lock_guard<std::mutex> lock(mStatsLock);

    auto key = std::make_pair(name, pid);
    auto it = mLookupTable.find(key);
    if (it == mLookupTable.end()) {
        LOG(INFO) << "WakeLock Stats: A stats entry for, \"" << name
                  << "\" was not found. This is most likely due to it being evicted.";
    } else {
        // Update existing entry
        WakeLockEntry updatedEntry = *(it->second);
        updatedEntry.updateOnRelease(epochTimeNow);

        // Make updated entry MRU
        mStats.erase(it->second);
        mStats.push_front(updatedEntry);
        mLookupTable[key] = mStats.begin();
    }
}

void WakeLockEntryList::getWakeLockStats(std::vector<WakeLockInfo>* aidl_return) const {
    std::lock_guard<std::mutex> lock(mStatsLock);

    for (const WakeLockEntry& entry : mStats) {
        aidl_return->push_back(entry.getWakeLockInfo());
    }
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
