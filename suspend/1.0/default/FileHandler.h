/*
 * Copyright (C) 2021 The Android Open Source Project
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
#ifndef ANDROID_SYSTEM_SUSPEND_FILE_HANDLER_H
#define ANDROID_SYSTEM_SUSPEND_FILE_HANDLER_H

#include <android-base/unique_fd.h>
#include <stdint.h>
#include <string>
#include <vector>

using ::android::base::unique_fd;
using ::std::string;

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

class FileHandler {
   public:
    enum FileId {
        SysPowerWakeupCount,
        SysPowerState,
        SysClassWakeup,
        SysPowerSuspendStats,
        SysKernelWakeupStats,
        SysKernelWakeupReasons,
        SysKernelSuspendTime,

        TotalFileIds
    };

    static FileHandler* Create(FileId, string, int);
    bool openFile(void);
    void setFilePathAndPermission(string, int);
    unique_fd fd;

   private:
    string path;
    int permission;
};

class SysFileHandler : public FileHandler {
   public:
    SysFileHandler(string, int);
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SUSPEND_FILE_HANDLER_H
