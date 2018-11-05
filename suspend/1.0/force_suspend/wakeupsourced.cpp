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
#include <android-base/strings.h>
#include <android/system/suspend/1.0/ISystemSuspend.h>
#include <android/system/suspend/1.0/IWakeupSource.h>

#include <dirent.h>
#include <iostream>
#include <string>
#include <unordered_map>

using android::sp;
using android::base::Join;
using android::base::ReadFileToString;
using android::base::Trim;
using android::base::WriteStringToFile;
using android::hardware::Return;
using android::hardware::Void;
using android::system::suspend::V1_0::ISystemSuspend;
using android::system::suspend::V1_0::IWakeupSource;

using std::string;
using std::unordered_map;
using WSStateMap = unordered_map<string, bool>;

// TODO: move std::filesystem once available in android.
template <class UnaryOp>
static void forEachFileInDir(const string& dir, const string& filename, UnaryOp op) {
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* de;
        while ((de = readdir(d))) {
            const string base = de->d_name;
            if (base[0] == '.') continue;

            const string abs_path = dir + "/" + base;
            switch (de->d_type) {
                case DT_REG:
                    if (base == filename) {
                        op(abs_path);
                    }
                    break;
                case DT_DIR:
                    forEachFileInDir(abs_path, filename, op);
                    break;
                default:
                    continue;
            }
        }
        closedir(d);
    }
}

static bool isWSenabled(const string& path) {
    string state;
    ReadFileToString(path, &state);
    state = Trim(state);
    return state == "enabled";
}

static bool enableWS(const string& path) {
    return WriteStringToFile("enabled", path);
}

static bool disableWS(const string& path) {
    return WriteStringToFile("disabled", path);
}

static WSStateMap getWakeupSourceState() {
    WSStateMap state{};
    forEachFileInDir("/sys", "wakeup", [&state](const string& path) {
        state[path] = isWSenabled(path);
        std::cout << path << "\t" << state[path] << std::endl;
    });
    return state;
}

class WakeupSource : public IWakeupSource {
   public:
    WakeupSource() : mWakeupSourceState() {}
    Return<void> enable();
    Return<void> disable();

   private:
    WSStateMap mWakeupSourceState;
};

Return<void> WakeupSource::enable() {
    if (mWakeupSourceState.empty()) {
        return Void();
    }
    for (const auto& [ws, state] : mWakeupSourceState) {
        state ? enableWS(ws) : disableWS(ws);
    }
    mWakeupSourceState.clear();
    return Void();
}

Return<void> WakeupSource::disable() {
    if (!mWakeupSourceState.empty()) {
        return Void();
    }
    mWakeupSourceState = getWakeupSourceState();
    for (const auto& [ws, state] : mWakeupSourceState) {
        disableWS(ws);
    }
    return Void();
}

int main() {
    sp<ISystemSuspend> suspendService = ISystemSuspend::getService();
    suspendService->registerWakeupSource(new WakeupSource());
    while (true) {
    }
}
