/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "WakeupList.h"

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

WakeupList::WakeupList(size_t capacity) : mWakeups(capacity) {}

void WakeupList::getWakeupStats(std::vector<WakeupInfo>* wakeups) const {
    std::scoped_lock lock(mLock);

    LruCache<std::string, long>::Iterator it(mWakeups);
    while (it.next()) {
        WakeupInfo w;
        w.name = it.key();
        w.count = it.value();

        wakeups->emplace_back(w);
    }
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android