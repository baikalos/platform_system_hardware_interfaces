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

void WakeLockEntryList::setCapacity(int capacity) {
    this->capacity = capacity;
}

void WakeLockEntryList::updateOnAcquire(std::string name, int pid, long epochTimeNow /* in us */) {
    std::lock_guard<std::mutex> lock(statsLock);

    auto it = lookupTable.find(name);
    if (it == lookupTable.end()) {
        // Evict LRU if stats is at capacity
        if (stats.size() == capacity) {
            auto it = stats.end();
            std::advance(it, -1);
            std::string key = it->getName();
            lookupTable.erase(key);
            stats.erase(it);
        }
        // Insert new entry as MRU
        stats.push_front(WakeLockEntry(name, pid, epochTimeNow));
        lookupTable[name] = stats.begin();
    } else {
        // Update existing entry
        WakeLockEntry updatedEntry = *(it->second);
        updatedEntry.updateOnAcquire(epochTimeNow);

        // Make updated entry MRU
        stats.erase(it->second);
        stats.push_front(updatedEntry);
        lookupTable[name] = stats.begin();
    }
}

void WakeLockEntryList::updateOnRelease(std::string name, long epochTimeNow /* in us */) {
    std::lock_guard<std::mutex> lock(statsLock);

    auto it = lookupTable.find(name);
    if (it == lookupTable.end()) {
        LOG(INFO) << "WakeLock Stats: A stats entry for, \"" << name
                  << "\" was not found. This is most likely due to it being evicted.";
    } else {
        // Update existing entry
        WakeLockEntry updatedEntry = *(it->second);
        updatedEntry.updateOnRelease(epochTimeNow);

        // Make updated entry MRU
        stats.erase(it->second);
        stats.push_front(updatedEntry);
        lookupTable[name] = stats.begin();
    }
}

void WakeLockEntryList::getWakeLockStats(std::vector<WakeLockInfo>* aidl_return) {
    std::lock_guard<std::mutex> lock(statsLock);

    for (const WakeLockEntry& entry : stats) {
        aidl_return->push_back(entry.getWakeLockInfo());
    }
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
