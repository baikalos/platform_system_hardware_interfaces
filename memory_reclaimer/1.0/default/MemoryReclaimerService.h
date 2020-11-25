/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_SYSTEM_SYSTEM_MEMORY_RECLAIMER_SERVICE_H
#define ANDROID_SYSTEM_SYSTEM_MEMORY_RECLAIMER_SERVICE_H

#include <aidl/android/system/memory_reclaimer/BnMemoryReclaimerService.h>

#include "MemoryReclaimer.h"

using aidl::android::system::memory_reclaimer::BnMemoryReclaimerService;
using android::system::memory_reclaimer::implementation::MemoryReclaimer;

namespace aidl::android::system::memory_reclaimer::V1_0 {

class MemoryReclaimerService : public BnMemoryReclaimerService {
   public:
    MemoryReclaimerService(std::shared_ptr<MemoryReclaimer> const& reclaimer)
        : reclaimer_(reclaimer){};
    ~MemoryReclaimerService() override = default;

    ndk::ScopedAStatus reclaim() override;

   private:
    std::shared_ptr<MemoryReclaimer> const reclaimer_;
};

}  // namespace aidl::android::system::memory_reclaimer::V1_0

#endif  // ANDROID_SYSTEM_SYSTEM_MEMORY_RECLAIMER_SERVICE_H
