/*
 * Copyright 2018 The Android Open Source Project
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/system/suspend/BnSuspendCallback.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/native_handle.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/HidlTransportSupport.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <future>
#include <string>
#include <thread>

#include "SuspendControlService.h"
#include "SystemSuspend.h"

using android::sp;
using android::base::Socketpair;
using android::base::unique_fd;
using android::base::WriteStringToFd;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::Return;
using android::hardware::Void;
using android::system::suspend::BnSuspendCallback;
using android::system::suspend::ISuspendControlService;
using android::system::suspend::WakeLockInfo;
using android::system::suspend::V1_0::getEpochTimeNow;
using android::system::suspend::V1_0::ISystemSuspend;
using android::system::suspend::V1_0::IWakeLock;
using android::system::suspend::V1_0::readFd;
using android::system::suspend::V1_0::SuspendControlService;
using android::system::suspend::V1_0::SystemSuspend;
using android::system::suspend::V1_0::TimestampType;
using android::system::suspend::V1_0::WakeLockType;
using namespace std::chrono_literals;

namespace android {

static constexpr char kServiceName[] = "TestService";
static constexpr char kControlServiceName[] = "TestControlService";

static bool isReadBlocked(int fd, int timeout_ms = 20) {
    struct pollfd pfd {
        .fd = fd, .events = POLLIN,
    };
    return poll(&pfd, 1, timeout_ms) == 0;
}

class SystemSuspendTestEnvironment : public ::testing::Environment {
   public:
    using Env = SystemSuspendTestEnvironment;
    static Env* Instance() {
        static Env* instance = new Env{};
        return instance;
    }

    SystemSuspendTestEnvironment() {
        Socketpair(SOCK_STREAM, &wakeupCountFds[0], &wakeupCountFds[1]);
        Socketpair(SOCK_STREAM, &stateFds[0], &stateFds[1]);
    }

    // Creates kernel wakelock stat file and writes stat to file.
    // Returns true on success, else false.
    bool writeStatToFile(int kernelWakelockFd, const std::string& fileName, int64_t stat) {
        unique_fd statFd{TEMP_FAILURE_RETRY(
            openat(kernelWakelockFd, fileName.c_str(), O_CREAT | O_CLOEXEC | O_RDWR, S_IRWXU))};
        if (statFd < 0) {
            PLOG(ERROR) << "SystemSuspend: Error opening " << fileName;
            return false;
        }

        if (!WriteStringToFd(std::to_string(stat), statFd.get())) {
            PLOG(ERROR) << "SystemSuspend: Error writing stat to " << fileName;
            return false;
        }

        return true;
    }

    // Creates a kernel wakelock directory and stats files.
    // Returns true on success else false.
    bool addKernelWakelock(int kernelWakelockStatsFd, const std::string& name,
                           int64_t activeCount = 42, int64_t activeTime = 42,
                           int64_t eventCount = 42, int64_t expireCount = 42,
                           int64_t lastChange = 42, int64_t maxTime = 42,
                           int64_t preventSuspendTime = 42, int64_t totalTime = 42,
                           int64_t wakeupCount = 42) {
        if ((mkdirat(kernelWakelockStatsFd, name.c_str(), S_IRWXU)) < 0) {
            PLOG(ERROR) << "SystemSuspend: Error creating directory for " << name << " wakelock";
            return false;
        }

        unique_fd kernelWakelockFd{TEMP_FAILURE_RETRY(
            openat(kernelWakelockStatsFd, name.c_str(), O_DIRECTORY | O_CLOEXEC | O_RDONLY))};
        if (kernelWakelockFd < 0) {
            PLOG(ERROR) << "SystemSuspend: Error opening " << name;
            return false;
        }

        int fd = kernelWakelockFd.get();

        return writeStatToFile(fd, "active_count", activeCount) &&
               writeStatToFile(fd, "active_time_ms", activeTime) &&
               writeStatToFile(fd, "event_count", eventCount) &&
               writeStatToFile(fd, "expire_count", expireCount) &&
               writeStatToFile(fd, "last_change_ms", lastChange) &&
               writeStatToFile(fd, "max_time_ms", maxTime) &&
               writeStatToFile(fd, "prevent_suspend_time_ms", preventSuspendTime) &&
               writeStatToFile(fd, "total_time_ms", totalTime) &&
               writeStatToFile(fd, "wakeup_count", wakeupCount);
    }

    // Adds kernel wake locks to temporary kernel wakelocks stats directory
    // Returns unique_fd to kernel wakelock stats directory on success else
    // unique fd is -1.
    unique_fd setUpKernelWakelockStats() {
        unique_fd kernelWakelockStatsFd{TEMP_FAILURE_RETRY(
            open(kernelWakelockStatsDir.path, O_DIRECTORY | O_CLOEXEC | O_RDONLY))};
        if (kernelWakelockStatsFd < 0) {
            PLOG(ERROR) << "SystemSuspend: Error opening " << kernelWakelockStatsDir.path;
            return unique_fd(-1);
        }

        // Add fake kernel wakelocks
        if (!addKernelWakelock(kernelWakelockStatsFd.get(), "fakeKwl")) {
            return unique_fd(-1);
        }
        return kernelWakelockStatsFd;
    }

    void registerTestService() {
        std::thread testService([this] {
            configureRpcThreadpool(1, true /* callerWillJoin */);

            sp<SuspendControlService> suspendControl = new SuspendControlService();
            auto controlStatus = ::android::defaultServiceManager()->addService(
                android::String16(kControlServiceName), suspendControl);
            if (android::OK != controlStatus) {
                LOG(FATAL) << "Unable to register service " << kControlServiceName << controlStatus;
            }

            // Create non-HW binder threadpool for SuspendControlService.
            sp<android::ProcessState> ps{android::ProcessState::self()};
            ps->startThreadPool();

            // Set up fake kernel wakelocks
            unique_fd kernelWakelockStatsFd = setUpKernelWakelockStats();
            if (kernelWakelockStatsFd < 0) {
                PLOG(FATAL) << "SystemSuspend: Failed to setup kernel wakelocks";
            }

            sp<ISystemSuspend> suspend = new SystemSuspend(
                std::move(wakeupCountFds[1]), std::move(stateFds[1]), 1 /* maxNativeStatsEntries */,
                std::move(kernelWakelockStatsFd) /* kernelWakelockStats Fd */,
                0ms /* baseSleepTime */, suspendControl);
            status_t status = suspend->registerAsService(kServiceName);
            if (android::OK != status) {
                LOG(FATAL) << "Unable to register service: " << status;
            }

            joinRpcThreadpool();
        });
        testService.detach();
    }

    virtual void SetUp() {
        registerTestService();
        ::android::hardware::details::waitForHwService(ISystemSuspend::descriptor, kServiceName);
        sp<ISystemSuspend> suspendService = ISystemSuspend::getService(kServiceName);
        ASSERT_NE(suspendService, nullptr) << "failed to get suspend service";

        sp<IBinder> control =
            android::defaultServiceManager()->getService(android::String16(kControlServiceName));
        ASSERT_NE(control, nullptr) << "failed to get the suspend control service";
        sp<ISuspendControlService> controlService = interface_cast<ISuspendControlService>(control);

        // Start auto-suspend.
        bool enabled = false;
        controlService->enableAutosuspend(&enabled);
        ASSERT_EQ(enabled, true) << "failed to start autosuspend";
    }

    unique_fd wakeupCountFds[2];
    unique_fd stateFds[2];
    unique_fd kernelWakelockStatsFd;

    TemporaryDir kernelWakelockStatsDir;
};

class SystemSuspendTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        ::android::hardware::details::waitForHwService(ISystemSuspend::descriptor, kServiceName);
        suspendService = ISystemSuspend::getService(kServiceName);
        ASSERT_NE(suspendService, nullptr) << "failed to get suspend service";

        sp<IBinder> control =
            android::defaultServiceManager()->getService(android::String16(kControlServiceName));
        ASSERT_NE(control, nullptr) << "failed to get the suspend control service";
        controlService = interface_cast<ISuspendControlService>(control);

        auto* environment = SystemSuspendTestEnvironment::Instance();
        wakeupCountFd = environment->wakeupCountFds[0];
        stateFd = environment->stateFds[0];

        // SystemSuspend HAL should not have written back to wakeupCountFd or stateFd yet.
        ASSERT_TRUE(isReadBlocked(wakeupCountFd));
        ASSERT_TRUE(isReadBlocked(stateFd));
    }

    virtual void TearDown() override {
        if (!isReadBlocked(wakeupCountFd)) readFd(wakeupCountFd);
        if (!isReadBlocked(stateFd)) readFd(stateFd).empty();
        ASSERT_TRUE(isReadBlocked(wakeupCountFd));
        ASSERT_TRUE(isReadBlocked(stateFd));
    }

    void unblockSystemSuspendFromWakeupCount() {
        std::string wakeupCount = std::to_string(rand());
        ASSERT_TRUE(WriteStringToFd(wakeupCount, wakeupCountFd));
    }

    bool isSystemSuspendBlocked(int timeout_ms = 20) { return isReadBlocked(stateFd, timeout_ms); }

    sp<IWakeLock> acquireWakeLock(const std::string& name = "TestLock") {
        return suspendService->acquireWakeLock(WakeLockType::PARTIAL, name);
    }

    size_t getActiveWakeLockCount() {
        std::vector<WakeLockInfo> wlStats;
        controlService->getWakeLockStats(&wlStats);
        return count_if(wlStats.begin(), wlStats.end(), [](auto entry) { return entry.isActive; });
    }

    void checkLoop(int numIter) {
        for (int i = 0; i < numIter; i++) {
            // Mock value for /sys/power/wakeup_count.
            std::string wakeupCount = std::to_string(rand());
            ASSERT_TRUE(WriteStringToFd(wakeupCount, wakeupCountFd));
            ASSERT_EQ(readFd(wakeupCountFd), wakeupCount)
                << "wakeup count value written by SystemSuspend is not equal to value given to it";
            ASSERT_EQ(readFd(stateFd), "mem")
                << "SystemSuspend failed to write correct sleep state.";
        }
    }

    // Returns true if wake lock is found else false.
    bool findWakeLockInfoByName(const std::vector<WakeLockInfo>& wlStats, const std::string& name,
                                WakeLockInfo* info) {
        auto it = std::find_if(wlStats.begin(), wlStats.end(),
                               [&name](const auto& x) { return x.name == name; });
        if (it != wlStats.end()) {
            *info = *it;
            return true;
        }
        return false;
    }

    sp<ISystemSuspend> suspendService;
    sp<ISuspendControlService> controlService;
    int stateFd;
    int wakeupCountFd;
};

// Tests that autosuspend thread can only be enabled once.
TEST_F(SystemSuspendTest, OnlyOneEnableAutosuspend) {
    bool enabled = false;
    controlService->enableAutosuspend(&enabled);
    ASSERT_EQ(enabled, false);
}

TEST_F(SystemSuspendTest, AutosuspendLoop) {
    checkLoop(5);
}

// Tests that upon WakeLock destruction SystemSuspend HAL is unblocked.
TEST_F(SystemSuspendTest, WakeLockDestructor) {
    {
        sp<IWakeLock> wl = acquireWakeLock();
        ASSERT_NE(wl, nullptr);
        unblockSystemSuspendFromWakeupCount();
        ASSERT_TRUE(isSystemSuspendBlocked());
    }
    ASSERT_FALSE(isSystemSuspendBlocked());
}

// Tests that upon WakeLock::release() SystemSuspend HAL is unblocked.
TEST_F(SystemSuspendTest, WakeLockRelease) {
    sp<IWakeLock> wl = acquireWakeLock();
    ASSERT_NE(wl, nullptr);
    unblockSystemSuspendFromWakeupCount();
    ASSERT_TRUE(isSystemSuspendBlocked());
    wl->release();
    ASSERT_FALSE(isSystemSuspendBlocked());
}

// Tests that multiple WakeLocks correctly block SystemSuspend HAL.
TEST_F(SystemSuspendTest, MultipleWakeLocks) {
    {
        sp<IWakeLock> wl1 = acquireWakeLock();
        ASSERT_NE(wl1, nullptr);
        ASSERT_TRUE(isSystemSuspendBlocked());
        unblockSystemSuspendFromWakeupCount();
        {
            sp<IWakeLock> wl2 = acquireWakeLock();
            ASSERT_NE(wl2, nullptr);
            ASSERT_TRUE(isSystemSuspendBlocked());
        }
        ASSERT_TRUE(isSystemSuspendBlocked());
    }
    ASSERT_FALSE(isSystemSuspendBlocked());
}

// Tests that upon thread deallocation WakeLock is destructed and SystemSuspend HAL is unblocked.
TEST_F(SystemSuspendTest, ThreadCleanup) {
    std::thread clientThread([this] {
        sp<IWakeLock> wl = acquireWakeLock();
        ASSERT_NE(wl, nullptr);
        unblockSystemSuspendFromWakeupCount();
        ASSERT_TRUE(isSystemSuspendBlocked());
    });
    clientThread.join();
    ASSERT_FALSE(isSystemSuspendBlocked());
}

// Test that binder driver correctly deallocates acquired WakeLocks, even if the client processs
// is terminated without ability to do clean up.
TEST_F(SystemSuspendTest, CleanupOnAbort) {
    ASSERT_EXIT(
        {
            sp<IWakeLock> wl = acquireWakeLock();
            ASSERT_NE(wl, nullptr);
            std::abort();
        },
        ::testing::KilledBySignal(SIGABRT), "");
    ASSERT_TRUE(isSystemSuspendBlocked());
    unblockSystemSuspendFromWakeupCount();
    // Timing of the wake lock clean-up after process death is scheduler-dependent.
    // Increase the timeout to avoid flakes.
    ASSERT_FALSE(isSystemSuspendBlocked(200));
}

// Test that getWakeLockStats has correct information about WakeLocks.
TEST_F(SystemSuspendTest, GetWakeLockStats) {
    TimestampType acquireTime = getEpochTimeNow();
    TimestampType releaseTime;
    std::string fakeWlName = "FakeLock";
    {
        sp<IWakeLock> fakeLock = acquireWakeLock(fakeWlName);
        std::vector<WakeLockInfo> wlStats;
        controlService->getWakeLockStats(&wlStats);
        // Size is 2 because 1 kernel wakelock was added in test setup.
        ASSERT_EQ(wlStats.size(), 2);

        // Native Wakelock Stats
        WakeLockInfo nwlInfo;
        ASSERT_TRUE(findWakeLockInfoByName(wlStats, fakeWlName, &nwlInfo));
        ASSERT_EQ(nwlInfo.name, fakeWlName);
        ASSERT_EQ(nwlInfo.activeCount, 1);
        // The updated timestamp is not deterministic. However, all SystemSuspend HAL calls run
        // in the order of microseconds, so in practice the timestamp should be within 1ms.
        ASSERT_LE(std::abs(nwlInfo.lastChange - acquireTime), 1);
        // The updated timestamp is not deterministic, however the time between
        // acquiring the wake lock and getting the stats is within 1ms.
        // Since,stats are reported in ms, these are effectively 0.
        ASSERT_EQ(nwlInfo.maxTime, 0);
        ASSERT_EQ(nwlInfo.totalTime, 0);
        ASSERT_EQ(nwlInfo.isActive, true);
        ASSERT_EQ(nwlInfo.activeTime, 0);
        ASSERT_FALSE(nwlInfo.isKernelWakelock);

        ASSERT_EQ(nwlInfo.pid, getpid());

        ASSERT_EQ(nwlInfo.eventCount, 0);
        ASSERT_EQ(nwlInfo.expireCount, 0);
        ASSERT_EQ(nwlInfo.preventSuspendTime, 0);
        ASSERT_EQ(nwlInfo.wakeupCount, 0);

        // Kernel Wakelock Stats
        WakeLockInfo kwlInfo;
        // "fakeKwl" added in setup
        ASSERT_TRUE(findWakeLockInfoByName(wlStats, "fakeKwl", &kwlInfo));
        ASSERT_EQ(kwlInfo.name, "fakeKwl");
        ASSERT_EQ(kwlInfo.activeCount, 42);
        ASSERT_EQ(kwlInfo.lastChange, 42);
        ASSERT_EQ(kwlInfo.maxTime, 42);
        ASSERT_EQ(kwlInfo.totalTime, 42);
        ASSERT_EQ(kwlInfo.isActive, true);
        ASSERT_EQ(kwlInfo.activeTime, 42);
        ASSERT_TRUE(kwlInfo.isKernelWakelock);

        ASSERT_EQ(kwlInfo.pid, -1);

        ASSERT_EQ(kwlInfo.eventCount, 42);
        ASSERT_EQ(kwlInfo.expireCount, 42);
        ASSERT_EQ(kwlInfo.preventSuspendTime, 42);
        ASSERT_EQ(kwlInfo.wakeupCount, 42);

        // We sleep so that the wake lock stats entry get updated with a different timestamp.
        std::this_thread::sleep_for(1s);
        releaseTime = getEpochTimeNow();
    }
    std::vector<WakeLockInfo> wlStats;
    controlService->getWakeLockStats(&wlStats);
    ASSERT_EQ(wlStats.size(), 2);

    // Native Wakelock Stats
    WakeLockInfo nwlInfo;
    ASSERT_TRUE(findWakeLockInfoByName(wlStats, fakeWlName, &nwlInfo));
    ASSERT_EQ(nwlInfo.name, fakeWlName);
    ASSERT_EQ(nwlInfo.activeCount, 1);
    ASSERT_LE(std::abs(nwlInfo.lastChange - releaseTime), 1);
    ASSERT_GE(nwlInfo.maxTime, 1000);
    ASSERT_GE(nwlInfo.totalTime, 1000);
    ASSERT_EQ(nwlInfo.isActive, false);
    ASSERT_EQ(nwlInfo.activeTime, 0);  // No longer active
    ASSERT_FALSE(nwlInfo.isKernelWakelock);

    ASSERT_EQ(nwlInfo.pid, getpid());

    ASSERT_EQ(nwlInfo.eventCount, 0);
    ASSERT_EQ(nwlInfo.expireCount, 0);
    ASSERT_EQ(nwlInfo.preventSuspendTime, 0);
    ASSERT_EQ(nwlInfo.wakeupCount, 0);

    // Kernel Wakelock Stats (There shouldn't be any changes here)
    WakeLockInfo kwlInfo;
    ASSERT_TRUE(findWakeLockInfoByName(wlStats, "fakeKwl", &kwlInfo));
    ASSERT_EQ(kwlInfo.name, "fakeKwl");
    ASSERT_EQ(kwlInfo.activeCount, 42);
    ASSERT_EQ(kwlInfo.lastChange, 42);
    ASSERT_EQ(kwlInfo.maxTime, 42);
    ASSERT_EQ(kwlInfo.totalTime, 42);
    ASSERT_EQ(kwlInfo.isActive, true);
    ASSERT_EQ(kwlInfo.activeTime, 42);
    ASSERT_TRUE(kwlInfo.isKernelWakelock);

    ASSERT_EQ(kwlInfo.pid, -1);

    ASSERT_EQ(kwlInfo.eventCount, 42);
    ASSERT_EQ(kwlInfo.expireCount, 42);
    ASSERT_EQ(kwlInfo.preventSuspendTime, 42);
    ASSERT_EQ(kwlInfo.wakeupCount, 42);
}

// Test that the least recently used native wake lock stats entry is evicted after a given
// threshold.
TEST_F(SystemSuspendTest, WakeLockStatsLruEviction) {
    std::string fakeWlName1 = "FakeLock1";
    std::string fakeWlName2 = "FakeLock2";

    acquireWakeLock(fakeWlName1);
    acquireWakeLock(fakeWlName2);

    std::vector<WakeLockInfo> wlStats;
    controlService->getWakeLockStats(&wlStats);

    // Max number of native stats entries was set to 1 in SystemSuspend
    // constructor, and 1 kernel wake lock added in test setup.
    ASSERT_EQ(wlStats.size(), 2);
    ASSERT_EQ(wlStats.begin()->name, fakeWlName2);

    WakeLockInfo wlInfo;
    ASSERT_TRUE(findWakeLockInfoByName(wlStats, fakeWlName2, &wlInfo));
    ASSERT_TRUE(findWakeLockInfoByName(wlStats, "fakeKwl", &wlInfo));
    ASSERT_FALSE(findWakeLockInfoByName(wlStats, fakeWlName1, &wlInfo));  // Evicted
}

// Stress test acquiring/releasing WakeLocks.
TEST_F(SystemSuspendTest, WakeLockStressTest) {
    // numThreads threads will acquire/release numLocks locks each.
    constexpr int numThreads = 10;
    constexpr int numLocks = 10000;
    std::thread tds[numThreads];

    for (int i = 0; i < numThreads; i++) {
        tds[i] = std::thread([this] {
            for (int j = 0; j < numLocks; j++) {
                sp<IWakeLock> wl1 = acquireWakeLock();
                sp<IWakeLock> wl2 = acquireWakeLock();
                wl2->release();
            }
        });
    }
    for (int i = 0; i < numThreads; i++) {
        tds[i].join();
    }
    // 1 active kernel wakelock was created in test setup.
    ASSERT_EQ(getActiveWakeLockCount(), 1);
}

// Callbacks are passed around as sp<>. However, mock expectations are verified when mock objects
// are destroyed, i.e. the test needs to control lifetime of the mock object.
// MockCallbackImpl can be destroyed independently of its wrapper MockCallback which is passed to
// SystemSuspend.
struct MockCallbackImpl {
    MOCK_METHOD1(notifyWakeup, binder::Status(bool));
};

class MockCallback : public BnSuspendCallback {
   public:
    MockCallback(MockCallbackImpl* impl) : mImpl(impl), mDisabled(false) {}
    binder::Status notifyWakeup(bool x) {
        return mDisabled ? binder::Status::ok() : mImpl->notifyWakeup(x);
    }
    // In case we pull the rug from under MockCallback, but SystemSuspend still has an sp<> to the
    // object.
    void disable() { mDisabled = true; }

   private:
    MockCallbackImpl* mImpl;
    bool mDisabled;
};

// Tests that nullptr can't be registered as callbacks.
TEST_F(SystemSuspendTest, RegisterInvalidCallback) {
    bool retval = false;
    controlService->registerCallback(nullptr, &retval);
    ASSERT_FALSE(retval);
}

// Tests that SystemSuspend HAL correctly notifies wakeup events.
TEST_F(SystemSuspendTest, CallbackNotifyWakeup) {
    constexpr int numWakeups = 5;
    MockCallbackImpl impl;
    // SystemSuspend should suspend numWakeup + 1 times. However, it might
    // only be able to notify numWakeup times. The test case might have
    // finished by the time last notification completes.
    EXPECT_CALL(impl, notifyWakeup).Times(testing::AtLeast(numWakeups));
    sp<MockCallback> cb = new MockCallback(&impl);
    bool retval = false;
    controlService->registerCallback(cb, &retval);
    ASSERT_TRUE(retval);
    checkLoop(numWakeups + 1);
    cb->disable();
}

// Tests that SystemSuspend HAL correctly deals with a dead callback.
TEST_F(SystemSuspendTest, DeadCallback) {
    ASSERT_EXIT(
        {
            sp<MockCallback> cb = new MockCallback(nullptr);
            bool retval = false;
            controlService->registerCallback(cb, &retval);
            ASSERT_TRUE(retval);
            std::exit(0);
        },
        ::testing::ExitedWithCode(0), "");

    // Dead process callback must still be dealt with either by unregistering it
    // or checking isOk() on every call.
    checkLoop(3);
}

// Callback that registers another callback.
class CbRegisteringCb : public BnSuspendCallback {
   public:
    CbRegisteringCb(sp<ISuspendControlService> controlService) : mControlService(controlService) {}
    binder::Status notifyWakeup(bool x) {
        sp<MockCallback> cb = new MockCallback(nullptr);
        cb->disable();
        bool retval = false;
        mControlService->registerCallback(cb, &retval);
        return binder::Status::ok();
    }

   private:
    sp<ISuspendControlService> mControlService;
};

// Tests that callback registering another callback doesn't result in a deadlock.
TEST_F(SystemSuspendTest, CallbackRegisterCallbackNoDeadlock) {
    sp<CbRegisteringCb> cb = new CbRegisteringCb(controlService);
    bool retval = false;
    controlService->registerCallback(cb, &retval);
    ASSERT_TRUE(retval);
    checkLoop(3);
}

}  // namespace android

int main(int argc, char** argv) {
    setenv("TREBLE_TESTING_OVERRIDE", "true", true);
    ::testing::AddGlobalTestEnvironment(android::SystemSuspendTestEnvironment::Instance());
    ::testing::InitGoogleMock(&argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
