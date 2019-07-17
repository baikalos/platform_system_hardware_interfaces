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

#include <android-base/file.h>
#include <android-base/logging.h>

using android::base::ReadFileToString;

static constexpr char kSysClassWakeup[] = "/sys/class/wakeup/";

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

static WakeLockInfo createEntry(const std::string& name, bool isKernelWakelock = true, int pid = 0,
                                TimestampType epochTimeNow = 0) {
    WakeLockInfo info;

    info.name = name;
    info.activeCount = (isKernelWakelock) ? 0 : 1;
    info.lastChange = epochTimeNow;
    info.maxTime = 0;
    info.totalTime = 0;
    info.isActive = (isKernelWakelock) ? false : true;
    info.isKernelWakelock = isKernelWakelock;

    info.pid = pid;
    info.activeSince = epochTimeNow;

    info.activeTime = 0;
    info.eventCount = 0;
    info.expireCount = 0;
    info.preventSuspendTime = 0;
    info.wakeupCount = 0;

    return info;
}

static void updateKernelEntry(WakeLockInfo& info, const std::string& name) {
    std::string path = kSysClassWakeup + name;
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(path.c_str()), &closedir);
    if (dir) {
        struct dirent* dp;
        while ((dp = readdir(dir.get()))) {
            std::string statName(dp->d_name);
            if ((statName == ".") || (statName == ".." || statName == "power" ||
                                      statName == "subsystem" || statName == "uevent")) {
                continue;
            }

            std::string statPath = path + "/" + statName;
            std::string valStr;
            if (!ReadFileToString(statPath, &valStr, true)) {
                LOG(ERROR) << "SystemSuspend: Failed to read \"" << statPath
                           << "\", errno: " << errno;
                continue;
            }
            long long statVal = std::stoll(valStr);

            if (statName == "active_count") {
                info.activeCount = statVal;
            } else if (statName == "active_time_ms") {
                info.activeTime = statVal;
            } else if (statName == "event_count") {
                info.eventCount = statVal;
            } else if (statName == "expire_count") {
                info.expireCount = statVal;
            } else if (statName == "last_change_ms") {
                info.lastChange = statVal;
            } else if (statName == "max_time_ms") {
                info.maxTime = statVal;
            } else if (statName == "prevent_suspend_time") {
                info.preventSuspendTime = statVal;
            } else if (statName == "total_time_ms") {
                info.totalTime = statVal;
            } else if (statName == "wakeup_count") {
                info.wakeupCount = statVal;
            }
        }
    }
}

static void getKernelWakelockStats(std::vector<WakeLockInfo>* aidl_return) {
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(kSysClassWakeup), &closedir);
    if (dir) {
        struct dirent* dp;
        while ((dp = readdir(dir.get()))) {
            std::string kwlName(dp->d_name);
            if ((kwlName == ".") || (kwlName == "..")) {
                continue;
            }
            WakeLockInfo entry = createEntry(kwlName);
            updateKernelEntry(entry, kwlName);
            aidl_return->emplace_back(entry);
        }
    }
}

TimestampType getEpochTimeNow() {
    auto timeSinceEpoch = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(timeSinceEpoch).count();
}

WakeLockEntryList::WakeLockEntryList(size_t capacity) : mCapacity(capacity) {}

/**
 * Evicts LRU from back of list if stats is at capacity.
 */
void WakeLockEntryList::evictIfFull() {
    if (mStats.size() == mCapacity) {
        auto evictIt = mStats.end();
        std::advance(evictIt, -1);
        auto evictKey = std::make_pair(evictIt->name, evictIt->pid);
        mLookupTable.erase(evictKey);
        mStats.erase(evictIt);
        LOG(ERROR) << "WakeLock Stats: Stats capacity met, consider adjusting capacity to "
                      "avoid stats eviction.";
    }
}

/**
 * Inserts entry as MRU.
 */
void WakeLockEntryList::insertEntry(WakeLockInfo entry) {
    auto key = std::make_pair(entry.name, entry.pid);
    mStats.emplace_front(entry);
    mLookupTable[key] = mStats.begin();
}

/**
 * Removes entry from the stats list.
 */
void WakeLockEntryList::deleteEntry(std::list<WakeLockInfo>::iterator entry) {
    mStats.erase(entry);
}

void WakeLockEntryList::updateOnAcquire(const std::string& name, int pid,
                                        TimestampType epochTimeNow) {
    std::lock_guard<std::mutex> lock(mStatsLock);

    auto key = std::make_pair(name, pid);
    auto it = mLookupTable.find(key);
    if (it == mLookupTable.end()) {
        evictIfFull();
        WakeLockInfo newEntry = createEntry(name, false /* kernel wl */, pid, epochTimeNow);
        insertEntry(newEntry);
    } else {
        auto staleEntry = it->second;
        WakeLockInfo updatedEntry = *staleEntry;

        // Update entry
        updatedEntry.isActive = true;
        updatedEntry.activeSince = epochTimeNow;
        updatedEntry.activeCount++;
        updatedEntry.lastChange = epochTimeNow;

        deleteEntry(staleEntry);
        insertEntry(updatedEntry);
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
        auto staleEntry = it->second;
        WakeLockInfo updatedEntry = *staleEntry;

        // Update entry
        updatedEntry.isActive = false;
        updatedEntry.maxTime =
            std::max(updatedEntry.maxTime, epochTimeNow - updatedEntry.activeSince);
        updatedEntry.totalTime += epochTimeNow - updatedEntry.lastChange;
        updatedEntry.lastChange = epochTimeNow;

        deleteEntry(staleEntry);
        insertEntry(updatedEntry);
    }
}
/**
 * Updates the native wakelock stats based on the current time.
 */
void WakeLockEntryList::updateNow() {
    std::lock_guard<std::mutex> lock(mStatsLock);

    TimestampType epochTimeNow = getEpochTimeNow();

    for (std::list<WakeLockInfo>::iterator it = mStats.begin(); it != mStats.end(); ++it) {
        if (it->isActive) {
            it->maxTime = std::max(it->maxTime, epochTimeNow - it->activeSince);
            it->totalTime += epochTimeNow - it->lastChange;
            it->lastChange = epochTimeNow;
        }
    }
}

void WakeLockEntryList::getWakeLockStats(std::vector<WakeLockInfo>* aidl_return) const {
    std::lock_guard<std::mutex> lock(mStatsLock);

    for (const WakeLockInfo& entry : mStats) {
        aidl_return->emplace_back(entry);
    }

    getKernelWakelockStats(aidl_return);
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
