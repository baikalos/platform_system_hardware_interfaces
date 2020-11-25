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

#define LOG_TAG "memory_reclaimerd"

#include <algorithm>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/system/memory_reclaimer/1.0/IMemoryReclaimer.h>
#include <hidl/LegacySupport.h>
#include "MemoryReclaimer.h"
#include "MemoryReclaimerHwService.h"
#include "MemoryReclaimerService.h"

using aidl::android::system::memory_reclaimer::V1_0::MemoryReclaimerService;

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using android::system::memory_reclaimer::implementation::MemoryReclaimerHwService;
using android::system::memory_reclaimer::V1_0::IMemoryReclaimer;

int main(int /* argc */, char* /* argv */[]) {
    const int threadPoolSize = std::max(std::thread::hardware_concurrency(), 1U);
    ALOGI("Starting memory reclaimer service");

    configureRpcThreadpool(1 /*threads*/, true /*willJoin*/);

    auto reclaimer = std::make_shared<MemoryReclaimer>(threadPoolSize);

    // AIDL
    std::shared_ptr<MemoryReclaimerService> reclaimerService =
        ndk::SharedRefBase::make<MemoryReclaimerService>(reclaimer);
    const std::string instance = std::string() + MemoryReclaimerService::descriptor + "/default";
    binder_status_t serviceStatus =
        AServiceManager_addService(reclaimerService->asBinder().get(), instance.c_str());
    if (serviceStatus != STATUS_OK) {
        ALOGE("Failed to register memory reclaimer service (AIDL)");
        return 1;
    }

    ABinderProcess_setThreadPoolMaxThreadCount(1 /*threads*/);
    ABinderProcess_startThreadPool();

    // HIDL
    android::sp<IMemoryReclaimer> reclaimerHwService = new MemoryReclaimerHwService(reclaimer);
    const android::status_t status = reclaimerHwService->registerAsService();
    if (status != ::android::OK) {
        ALOGE("Failed to register memory reclaimer service (HIDL)");
        return 1;
    }

    joinRpcThreadpool();

    ALOGE("Memory reclaimer service exiting");

    return 1;
}
