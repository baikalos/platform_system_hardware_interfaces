/*
 * Copyright 2021 The Android Open Source Project
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

#include "FileHandler.h"

#include <cutils/native_handle.h>
#include <fcntl.h>
#include <unistd.h>

using ::std::string;
using namespace std;

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

SysFileHandler::SysFileHandler(string path, int permission) {
    setFilePathAndPermission(path, permission);
}

bool FileHandler::openFile(void) {
    unique_fd tempFd{TEMP_FAILURE_RETRY(open(path.c_str(), permission))};
    fd = std::move(tempFd);
    return (fd > 0);
}

void FileHandler::setFilePathAndPermission(string path, int permission) {
    this->path = path;
    this->permission = permission;
}

FileHandler* FileHandler::Create(FileId id, string path, int permission) {
    switch (id) {
        case SysPowerWakeupCount:
        case SysPowerState:
        case SysClassWakeup:
        case SysPowerSuspendStats:
        case SysKernelWakeupStats:
        case SysKernelWakeupReasons:
        case SysKernelSuspendTime:
            return new SysFileHandler(path, permission);

        default:
            return NULL;
    }
}

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android
