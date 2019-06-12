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

#include "WakeLockEntry.h"

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

WakeLockEntry::WakeLockEntry(std::string name, int pid, long epochTimeNow /* in us */)
    : name(name), pid(pid) {
    updateOnAcquire(epochTimeNow);
}

std::string WakeLockEntry::getName() {
    return name;
}

void WakeLockEntry::updateOnAcquire(long epochTimeNow /* in us */) {
    isActive = true;
    activeSince = epochTimeNow;
    activeCount++;
    lastChange = epochTimeNow;
}

void WakeLockEntry::updateOnRelease(long epochTimeNow /* in us */) {
    isActive = false;
    maxTime = std::max(maxTime, epochTimeNow - activeSince);
    totalTime += epochTimeNow - activeSince;
    lastChange = epochTimeNow;
}

WakeLockInfo WakeLockEntry::getWakeLockInfo() const {
    WakeLockInfo wakeLockInfo;
    wakeLockInfo.name = name;
    wakeLockInfo.pid = pid;
    wakeLockInfo.activeSince = activeSince;
    wakeLockInfo.activeCount = activeCount;
    wakeLockInfo.lastChange = lastChange;
    wakeLockInfo.maxTime = maxTime;
    wakeLockInfo.totalTime = totalTime;
    wakeLockInfo.isActive = isActive;
    wakeLockInfo.eventCount = eventCount;
    wakeLockInfo.wakeupCount = wakeupCount;
    wakeLockInfo.expireCount = expireCount;
    wakeLockInfo.preventSuspendTime = preventSuspendTime;
    return wakeLockInfo;
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
