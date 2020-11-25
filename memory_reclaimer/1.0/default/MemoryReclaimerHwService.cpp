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

#include <functional>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <meminfo/procmeminfo.h>

#include <hwbinder/IPCThreadState.h>

#include "MemoryReclaimerHwService.h"

using android::hardware::IPCThreadState;
using android::hardware::Void;

namespace android::system::memory_reclaimer::implementation {

Return<void> MemoryReclaimerHwService::reclaim() {
    pid_t pid = IPCThreadState::self()->getCallingPid();

    reclaimer_->reclaimAndWriteBack(pid);

    return Void();
}

}  // namespace android::system::memory_reclaimer::implementation
