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

#include "SystemSuspend.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <hidl/Status.h>
#include <hwbinder/IPCThreadState.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <string>
#include <thread>

using ::android::base::ReadFileToString;
using ::android::base::WriteStringToFd;
using ::android::hardware::Void;
using ::std::endl;
using ::std::string;

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

static const char kSleepState[] = "mem";
// TODO(b/128923994): we only need /sys/power/wake_[un]lock to export debugging info via
// /sys/kernel/debug/wakeup_sources.
static constexpr char kSysPowerWakeLock[] = "/sys/power/wake_lock";
static constexpr char kSysPowerWakeUnlock[] = "/sys/power/wake_unlock";

// This function assumes that data in fd is small enough that it can be read in one go.
// We use this function instead of the ones available in libbase because it doesn't block
// indefinitely when reading from socket streams which are used for testing.
string readFd(int fd) {
    char buf[BUFSIZ];
    ssize_t n = TEMP_FAILURE_RETRY(read(fd, &buf[0], sizeof(buf)));
    if (n < 0) return "";
    return string{buf, static_cast<size_t>(n)};
}

static inline int getCallingPid() {
    return ::android::hardware::IPCThreadState::self()->getCallingPid();
}

WakeLock::WakeLock(SystemSuspend* systemSuspend, const string& name, int pid)
    : mReleased(), mSystemSuspend(systemSuspend), mName(name), mPid(pid) {
    mSystemSuspend->incSuspendCounter(mName);
}

WakeLock::~WakeLock() {
    releaseOnce();
}

Return<void> WakeLock::release() {
    releaseOnce();
    return Void();
}

void WakeLock::releaseOnce() {
    std::call_once(mReleased, [this]() {
        mSystemSuspend->decSuspendCounter(mName);
        mSystemSuspend->updateWakeLockStatOnRelease(mName, mPid, getEpochTimeNow());
    });
}

SystemSuspend::SystemSuspend(unique_fd wakeupCountFd, unique_fd stateFd, size_t maxStatsEntries,
                             std::chrono::milliseconds baseSleepTime,
                             const sp<SuspendControlService>& controlService,
                             bool useSuspendCounter)
    : mSuspendCounter(0),
      mWakeupCountFd(std::move(wakeupCountFd)),
      mStateFd(std::move(stateFd)),
      mBaseSleepTime(baseSleepTime),
      mSleepTime(baseSleepTime),
      mControlService(controlService),
      mStatsList(maxStatsEntries),
      mUseSuspendCounter(useSuspendCounter),
      mWakeLockFd(-1),
      mWakeUnlockFd(-1) {
    mControlService->setSuspendService(this);

    if (!mUseSuspendCounter) {
        mWakeLockFd.reset(TEMP_FAILURE_RETRY(open(kSysPowerWakeLock, O_CLOEXEC | O_RDWR)));
        if (mWakeLockFd < 0) {
            PLOG(ERROR) << "error opening " << kSysPowerWakeLock;
        }
        mWakeUnlockFd.reset(TEMP_FAILURE_RETRY(open(kSysPowerWakeUnlock, O_CLOEXEC | O_RDWR)));
        if (mWakeUnlockFd < 0) {
            PLOG(ERROR) << "error opening " << kSysPowerWakeUnlock;
        }
    }
}

bool SystemSuspend::enableAutosuspend() {
    static bool initialized = false;
    if (initialized) {
        LOG(ERROR) << "Autosuspend already started.";
        return false;
    }

    initAutosuspend();
    initialized = true;
    return true;
}

bool SystemSuspend::forceSuspend() {
    //  We are forcing the system to suspend. This particular call ignores all
    //  existing wakelocks (full or partial). It does not cancel the wakelocks
    //  or reset mSuspendCounter, it just ignores them.  When the system
    //  returns from suspend, the wakelocks and SuspendCounter will not have
    //  changed.
    auto counterLock = std::unique_lock(mCounterLock);
    bool success = WriteStringToFd(kSleepState, mStateFd);
    counterLock.unlock();

    if (!success) {
        PLOG(VERBOSE) << "error writing to /sys/power/state for forceSuspend";
    }
    return success;
}

Return<sp<IWakeLock>> SystemSuspend::acquireWakeLock(WakeLockType /* type */,
                                                     const hidl_string& name) {
    auto pid = getCallingPid();
    auto timeNow = getEpochTimeNow();
    IWakeLock* wl = new WakeLock{this, name, pid};
    mStatsList.updateOnAcquire(name, pid, timeNow);
    return wl;
}

void SystemSuspend::incSuspendCounter(const string& name) {
    auto l = std::lock_guard(mCounterLock);
    if (mUseSuspendCounter) {
        mSuspendCounter++;
    } else {
        if (!WriteStringToFd(name, mWakeLockFd)) {
            PLOG(ERROR) << "error writing " << name << " to " << kSysPowerWakeLock;
        }
    }
}

void SystemSuspend::decSuspendCounter(const string& name) {
    auto l = std::lock_guard(mCounterLock);
    if (mUseSuspendCounter) {
        if (--mSuspendCounter == 0) {
            mCounterCondVar.notify_one();
        }
    } else {
        if (!WriteStringToFd(name, mWakeUnlockFd)) {
            PLOG(ERROR) << "error writing " << name << " to " << kSysPowerWakeUnlock;
        }
    }
}

void SystemSuspend::initAutosuspend() {
    std::thread autosuspendThread([this] {
        while (true) {
            std::this_thread::sleep_for(mSleepTime);
            lseek(mWakeupCountFd, 0, SEEK_SET);
            const string wakeupCount = readFd(mWakeupCountFd);
            if (wakeupCount.empty()) {
                PLOG(ERROR) << "error reading from /sys/power/wakeup_count";
                continue;
            }

            auto counterLock = std::unique_lock(mCounterLock);
            mCounterCondVar.wait(counterLock, [this] { return mSuspendCounter == 0; });
            // The mutex is locked and *MUST* remain locked until we write to /sys/power/state.
            // Otherwise, a WakeLock might be acquired after we check mSuspendCounter and before we
            // write to /sys/power/state.

            if (!WriteStringToFd(wakeupCount, mWakeupCountFd)) {
                PLOG(VERBOSE) << "error writing from /sys/power/wakeup_count";
                continue;
            }
            bool success = WriteStringToFd(kSleepState, mStateFd);
            counterLock.unlock();

            if (!success) {
                PLOG(VERBOSE) << "error writing to /sys/power/state";
            }

            mControlService->notifyWakeup(success);

            updateSleepTime(success);
        }
    });
    autosuspendThread.detach();
    LOG(INFO) << "automatic system suspend enabled";
}

void SystemSuspend::updateSleepTime(bool success) {
    static constexpr std::chrono::milliseconds kMaxSleepTime = 1min;
    if (success) {
        mSleepTime = mBaseSleepTime;
        return;
    }
    // Double sleep time after each failure up to one minute.
    mSleepTime = std::min(mSleepTime * 2, kMaxSleepTime);
}

void SystemSuspend::updateWakeLockStatOnRelease(const std::string& name, int pid,
                                                TimestampType epochTimeNow) {
    mStatsList.updateOnRelease(name, pid, epochTimeNow);
}

const WakeLockEntryList& SystemSuspend::getStatsList() const {
    return mStatsList;
}

void SystemSuspend::updateStatsNow() {
    mStatsList.updateNow();
}

std::string SystemSuspend::debugUsage() {
    std::stringstream ss;

    ss << "Usage: adb shell lshal debug \\" << endl
       << "           android.system.suspend@1.0::ISystemSuspend/default [options...]" << endl
       << endl
       << "   Options:" << endl
       << "       --wakelocks  : return wakelock stats data." << endl
       << "       --traces     : return SystemSuspend kernel stack traces data." << endl
       << "       --all        : return all debug data, regardless of any other option(s)" << endl
       << "                      provided. Same as providing no (valid) options." << endl
       << "       --help or -h : prints this message, regardless of any other option(s)" << endl
       << "                      provided." << endl
       << endl;

    return ss.str();
}

/**
 * Write SystemSuspend debug info to handle's fd.
 *
 * Usage:
 *    adb shell lshal debug \
 *        android.system.suspend@1.0::ISystemSuspend/default [options...]
 *
 *    Options:
 *        --wakelocks  : return wakelock stats data.
 *        --traces     : return SystemSuspend kernel stack traces data.
 *        --all        : return all debug data, regardless of any other option(s)
 *                       provided. Same as providing no (valid) options.
 *        --help or -h : prints this message, regardless of any other option(s)
 *                       provided.
 */
Return<void> SystemSuspend::debug(const hidl_handle& handle, const hidl_vec<hidl_string>& options) {
    if (handle == nullptr || handle->numFds < 1 || handle->data[0] < 0) {
        LOG(ERROR) << "SystemSuspend::debug: No valid fd.";
        return Void();
    }
    int fd = handle->data[0];

    bool all = true;
    bool traces = false;
    bool wakelocks = false;
    bool help = false;

    // Parse options
    for (const hidl_string& option : options) {
        std::string opt(option);
        if (opt == "--traces") {
            traces = true;
            all = false;
        } else if (opt == "--wakelocks") {
            wakelocks = true;
            all = false;
        } else if (opt == "--all") {
            all = true;
        } else if (opt == "-h" || opt == "--help") {
            help = true;
            break;
        }
    }

    if (help) {
        std::string usage = debugUsage();
        if (!WriteStringToFd(usage, fd)) {
            LOG(WARNING) << "SystemSuspend::debug: Failed to write usage to fd, errno: " << errno;
            return Void();
        }
    } else {
        if (traces || all) {
            std::string traces = getStackTraces();

            if (!WriteStringToFd(traces, fd)) {
                LOG(WARNING) << "SystemSuspend::debug: Failed to write traces to fd, errno: "
                             << errno;
                return Void();
            }
        }

        if (wakelocks || all) {
            // TODO(136689376): Get wake lock stats
        }
    }

    fsync(fd);
    return Void();
}

std::string SystemSuspend::getStackTraces() {
    std::string header = "SystemSuspend Kernel Stack Traces sysTid=";
    std::stringstream ss;

    auto tids = getTids();
    if (tids.empty()) {
        LOG(WARNING) << "SystemSuspend::getStackTraces: Failed to retrieve tids.";
        return ss.str();
    }

    for (int tid : tids) {
        std::string path = "/proc/self/task/" + std::to_string(tid) + "/stack";
        string stackStr;
        if (!ReadFileToString(path, &stackStr, true)) {
            LOG(WARNING) << "SystemSuspend::getStackTraces: Failed to read \"" << path
                         << "\", errno: " << errno;
            return ss.str();
        }

        ss << header << tid << endl;
        ss << stackStr << endl;
    }

    return ss.str();
}

std::vector<int> SystemSuspend::getTids() {
    std::vector<int> tids;
    DIR* dir = opendir("/proc/self/task");
    struct dirent* dp;
    if (dir) {
        while ((dp = readdir(dir))) {
            std::string name(dp->d_name);
            if ((name == ".") || (name == "..")) {
                continue;
            }
            tids.push_back(std::stoi(name));
        }
        closedir(dir);
    }
    return tids;
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
