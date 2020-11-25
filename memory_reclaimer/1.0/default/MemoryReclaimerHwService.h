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

#ifndef ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_MEMORY_RECLAIMER_HWSERVICE_H
#define ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_MEMORY_RECLAIMER_HWSERVICE_H

// #include <memory>
#include <shared_mutex>

#include <android/system/memory_reclaimer/1.0/IMemoryReclaimer.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include "MemoryReclaimer.h"

namespace android::system::memory_reclaimer::implementation {

using android::hardware::Return;

struct MemoryReclaimerHwService : public V1_0::IMemoryReclaimer {
    MemoryReclaimerHwService(std::shared_ptr<MemoryReclaimer> const& reclaimer)
        : reclaimer_(reclaimer) {}

    Return<void> reclaim() override;

   private:
    std::shared_ptr<MemoryReclaimer> const reclaimer_;
};

}  // namespace android::system::memory_reclaimer::implementation

#endif  // ANDROID_SYSTEM_MEMORY_RECLAIMER_V1_0_MEMORY_RECLAIMER_HWSERVICE_H
