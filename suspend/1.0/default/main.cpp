/*
 * Copyright 2019 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/native_handle.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/ProcessState.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <SuspendProperties.sysprop.h>

#include "SuspendControlService.h"
#include "SystemSuspend.h"
#include "SystemSuspendAidl.h"
#include "SystemSuspendHidl.h"

using aidl::android::system::suspend::SystemSuspendAidl;
using android::sp;
using android::status_t;
using android::String16;
using android::base::Socketpair;
using android::base::unique_fd;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::system::suspend::V1_0::ISystemSuspend;
using android::system::suspend::V1_0::SleepTimeConfig;
using android::system::suspend::V1_0::SuspendControlService;
using android::system::suspend::V1_0::SuspendControlServiceInternal;
using android::system::suspend::V1_0::SystemSuspend;
using android::system::suspend::V1_0::SystemSuspendHidl;
using namespace std::chrono_literals;
using namespace ::android::sysprop;

static constexpr size_t kStatsCapacity = 1000;
static constexpr char kSysClassWakeup[] = "/sys/class/wakeup";
static constexpr char kSysPowerSuspendStats[] = "/sys/power/suspend_stats";
static constexpr char kSysPowerWakeupCount[] = "/sys/power/wakeup_count";
static constexpr char kSysPowerState[] = "/sys/power/state";
// TODO(b/120445600): Use upstream mechanism for wakeup reasons once available
static constexpr char kSysKernelWakeupReasons[] = "/sys/kernel/wakeup_reasons/last_resume_reason";
static constexpr char kSysKernelSuspendTime[] = "/sys/kernel/wakeup_reasons/last_suspend_time";

static constexpr uint32_t kDefaultMaxSleepTimeMillis = 500;
static constexpr uint32_t kDefaultBaseSleepTimeMillis = 10;
static constexpr double kDefaultSleepTimeScaleFactor = 2.0;
static constexpr uint32_t kDefaultBackoffThresholdCount = 1;
static constexpr uint32_t kDefaultShortSuspendThresholdMillis = 50;
static constexpr bool kDefaultFailedSuspendBackoffEnabled = true;
static constexpr bool kDefaultShortSuspendBackoffEnabled = true;

int main() {
    unique_fd wakeupCountFd{TEMP_FAILURE_RETRY(open(kSysPowerWakeupCount, O_CLOEXEC | O_RDWR))};
    if (wakeupCountFd < 0) {
        PLOG(ERROR) << "error opening " << kSysPowerWakeupCount;
    }
    unique_fd stateFd{TEMP_FAILURE_RETRY(open(kSysPowerState, O_CLOEXEC | O_RDWR))};
    if (stateFd < 0) {
        PLOG(ERROR) << "error opening " << kSysPowerState;
    }
    unique_fd kernelWakelockStatsFd{
        TEMP_FAILURE_RETRY(open(kSysClassWakeup, O_DIRECTORY | O_CLOEXEC | O_RDONLY))};
    if (kernelWakelockStatsFd < 0) {
        PLOG(ERROR) << "SystemSuspend: Error opening " << kSysClassWakeup;
    }
    unique_fd suspendStatsFd{
        TEMP_FAILURE_RETRY(open(kSysPowerSuspendStats, O_DIRECTORY | O_CLOEXEC | O_RDONLY))};
    if (suspendStatsFd < 0) {
        PLOG(ERROR) << "SystemSuspend: Error opening " << kSysPowerSuspendStats;
    }
    unique_fd wakeupReasonsFd{
        TEMP_FAILURE_RETRY(open(kSysKernelWakeupReasons, O_CLOEXEC | O_RDONLY))};
    if (wakeupReasonsFd < 0) {
        PLOG(ERROR) << "SystemSuspend: Error opening " << kSysKernelWakeupReasons;
    }
    unique_fd suspendTimeFd{TEMP_FAILURE_RETRY(open(kSysKernelSuspendTime, O_CLOEXEC | O_RDONLY))};
    if (suspendTimeFd < 0) {
        PLOG(ERROR) << "SystemSuspend: Error opening " << kSysKernelSuspendTime;
    }

    // If either /sys/power/wakeup_count or /sys/power/state fail to open, we construct
    // SystemSuspend with blocking fds. This way this process will keep running, handle wake lock
    // requests, collect stats, but won't suspend the device. We want this behavior on devices
    // (hosts) where system suspend should not be handles by Android platform e.g. ARC++, Android
    // virtual devices.
    if (wakeupCountFd < 0 || stateFd < 0) {
        // This will block all reads/writes to these fds from the suspend thread.
        Socketpair(SOCK_STREAM, &wakeupCountFd, &stateFd);
    }

    SleepTimeConfig sleepTimeConfig = {
        .baseSleepTime = std::chrono::milliseconds(
            SuspendProperties::base_sleep_time_millis().value_or(kDefaultBaseSleepTimeMillis)),
        .maxSleepTime = std::chrono::milliseconds(
            SuspendProperties::max_sleep_time_millis().value_or(kDefaultMaxSleepTimeMillis)),
        .sleepTimeScaleFactor =
            SuspendProperties::sleep_time_scale_factor().value_or(kDefaultSleepTimeScaleFactor),
        .backoffThreshold =
            SuspendProperties::backoff_threshold_count().value_or(kDefaultBackoffThresholdCount),
        .shortSuspendThreshold =
            std::chrono::milliseconds(SuspendProperties::short_suspend_threshold_millis().value_or(
                kDefaultShortSuspendThresholdMillis)),
        .failedSuspendBackoffEnabled = SuspendProperties::failed_suspend_backoff_enabled().value_or(
            kDefaultFailedSuspendBackoffEnabled),
        .shortSuspendBackoffEnabled = SuspendProperties::short_suspend_backoff_enabled().value_or(
            kDefaultShortSuspendBackoffEnabled),
    };

    configureRpcThreadpool(1, true /* callerWillJoin */);

    sp<SuspendControlService> suspendControl = new SuspendControlService();
    auto controlStatus =
        android::defaultServiceManager()->addService(String16("suspend_control"), suspendControl);
    if (controlStatus != android::OK) {
        LOG(FATAL) << "Unable to register suspend_control service: " << controlStatus;
    }

    sp<SuspendControlServiceInternal> suspendControlInternal = new SuspendControlServiceInternal();
    controlStatus = android::defaultServiceManager()->addService(
        String16("suspend_control_internal"), suspendControlInternal);
    if (controlStatus != android::OK) {
        LOG(FATAL) << "Unable to register suspend_control_internal service: " << controlStatus;
    }

    // Create non-HW binder threadpool for SuspendControlService.
    sp<android::ProcessState> ps{android::ProcessState::self()};
    ps->startThreadPool();

    sp<SystemSuspend> suspend = new SystemSuspend(
        std::move(wakeupCountFd), std::move(stateFd), std::move(suspendStatsFd), kStatsCapacity,
        std::move(kernelWakelockStatsFd), std::move(wakeupReasonsFd), std::move(suspendTimeFd),
        sleepTimeConfig, suspendControl, suspendControlInternal, true /* mUseSuspendCounter*/);

    std::shared_ptr<SystemSuspendAidl> suspendAidl =
        ndk::SharedRefBase::make<SystemSuspendAidl>(suspend.get());
    const std::string suspendAidlInstance =
        std::string() + SystemSuspendAidl::descriptor + "/default";
    auto aidlStatus =
        AServiceManager_addService(suspendAidl->asBinder().get(), suspendAidlInstance.c_str());
    CHECK_EQ(aidlStatus, STATUS_OK)
        << "Unable to register system-suspend AIDL service: " << aidlStatus;

    sp<SystemSuspendHidl> suspendHidl = new SystemSuspendHidl(suspend.get());
    status_t hidlStatus = suspendHidl->registerAsService();
    if (android::OK != hidlStatus) {
        LOG(INFO) << "system-suspend HIDL hal not supported, use the AIDL suspend hal for "
                     "requesting wakelocks";
    }

    joinRpcThreadpool();
    std::abort(); /* unreachable */
}
