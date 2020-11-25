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

#include <inttypes.h>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <log/log.h>
#include <meminfo/procmeminfo.h>

#include "MemoryReclaimer.h"

using android::base::ReadFileToString;
using android::base::StringPrintf;
using android::base::Trim;
using android::base::WriteStringToFile;
using android::meminfo::ProcMemInfo;

namespace android::system::memory_reclaimer::implementation {

void MemoryReclaimer::reclaimAndWriteBackInternal(pid_t pid) {
    std::string processName;

    if (ReadFileToString(StringPrintf("/proc/%d/comm", pid), &processName)) {
        processName = Trim(processName);  // trailing '\n'
    } else {
        processName = "<unknown>";
    }

    const char* name = processName.c_str();

    ALOGI("Reclaiming memory for pid %d (%s)", pid, name);

    std::string reclaimPath = StringPrintf("/proc/%d/reclaim", pid);
    if (!WriteStringToFile("all", reclaimPath)) {
        ALOGE("Failed to reclaim memory for pid %d (%s)", pid, name);

        return;
    }

    ALOGI("Done reclaiming memory for pid %d (%s)", pid, name);

    ProcMemInfo serviceMemInfo(pid);

    const std::vector<uint64_t>& swapOffsets = serviceMemInfo.SwapOffsets();
    const size_t blocks = swapOffsets.size();
    if (blocks == 0) {
        ALOGI("No blocks to write back for pid %d (%s)", pid, name);
        return;
    }

    ALOGI("Writing back %zu blocks for pid %d (%s)", blocks, pid, name);

    static const std::string zramWriteback("/sys/block/zram0/writeback");
    for (uint64_t offset : swapOffsets) {
        std::string pageIndex = StringPrintf("page_index=%" PRIu64, offset);
        if (!WriteStringToFile(pageIndex, zramWriteback)) {
            ALOGE("Failed to write back ZRAM for swap offset %" PRIu64 ", pid %d (%s)", offset, pid,
                  name);

            return;
        }
    }

    ALOGI("Done writing back ZRAM for pid %d (%s)", pid, name);

    return;
}

void MemoryReclaimer::reclaimAndWriteBack(pid_t pid) {
    threadPool_->schedule(std::bind(reclaimAndWriteBackInternal, pid));
}

void MemoryReclaimer::shutdown() {
    threadPool_->shutdown();
}

}  // namespace android::system::memory_reclaimer::implementation
