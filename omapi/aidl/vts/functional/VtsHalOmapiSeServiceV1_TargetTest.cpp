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

#include <aidl/android/system/omapi/ISecureElementChannel.h>
#include <aidl/android/system/omapi/ISecureElementListener.h>
#include <aidl/android/system/omapi/ISecureElementReader.h>
#include <aidl/android/system/omapi/ISecureElementService.h>
#include <aidl/android/system/omapi/ISecureElementSession.h>
#include <aidl/android/system/omapi/SecureElementErrorCode.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <utils/String16.h>

using namespace std;
using namespace ::testing;
using namespace android;

int main(int argc, char** argv) {
    InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

namespace {

class OMAPISEServiceHalTest : public TestWithParam<std::string> {
   protected:
    void loadVendorStableReaders() {
        std::vector<std::string> readers = {};

        if (omapiSecureService() != NULL) {
            omapiSecureService()->getReaders(&readers);

            LOG(INFO) << "OMAPI: Readers: ";
            for (auto readerName : readers) {
                LOG(INFO) << readerName;
                std::shared_ptr<::aidl::android::system::omapi::ISecureElementReader> reader;
                omapiSecureService()->getReader(readerName, &reader);

                mVSReaders[readerName] = reader;
            }
        }
    }

    void SetUp() override {
        ::ndk::SpAIBinder ks2Binder(AServiceManager_getService(omapiServiceName));
        omapiSeService = aidl::android::system::omapi::ISecureElementService::fromBinder(ks2Binder);
        ASSERT_TRUE(omapiSeService);

        /*::ndk::SpAIBinder ks2Binder(AServiceManager_getService(omapiSeReaderName));
        omapiSeReader = aidl::android::system::omapi::ISecureElementReader::fromBinder(ks2Binder);
        ASSERT_TRUE(omapiSeReader);*/

        loadVendorStableReaders();
    }

    void TearDown() override {}

    bool isDebuggableBuild() {
        char value[PROPERTY_VALUE_MAX] = {0};
        property_get("ro.system.build.type", value, "");
        if (strcmp(value, "userdebug") == 0) {
            return true;
        }
        if (strcmp(value, "eng") == 0) {
            return true;
        }
        return false;
    }

    std::shared_ptr<aidl::android::system::omapi::ISecureElementService> omapiSecureService() {
        return omapiSeService;
    }

    /*
    std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> omapiSecureReader() {
        return omapiSeReader;
    }*/

    static inline std::string const UICC_READER_PREFIX = "SIM";
    static inline std::string const ESE_READER_PREFIX = "eSE";
    static inline std::string const SD_READER_PREFIX = "SD";

    constexpr static const char omapiServiceName[] =
        "android.system.omapi.ISecureElementService/default";
    /*constexpr static const char omapiSeReaderName[] =
        "android.system.omapi.ISecureElementReader/default";*/

    std::shared_ptr<aidl::android::system::omapi::ISecureElementService> omapiSeService;
    // std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> omapiSeReader;

    std::map<std::string, std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>>
        mVSReaders = {};
};

TEST_P(OMAPISEServiceHalTest, TestGetReaders) {
    bool result = true;
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> uiccReaders =
        {};
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> eseReaders =
        {};
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> sdReaders = {};

    for (const auto& [name, reader] : mVSReaders) {
        bool status = false;
        LOG(INFO) << "Name of the reader: " << name;

        if (reader) {
            reader->isSecureElementPresent(&status);
        }
        ASSERT_TRUE(status);

        if (!(name.rfind(UICC_READER_PREFIX, 0) || name.rfind(ESE_READER_PREFIX, 0) ||
              name.rfind(SD_READER_PREFIX))) {
            LOG(ERROR) << "Incorrect Reader name";
            return;
        }

        if (name.rfind(UICC_READER_PREFIX, 0)) {
            uiccReaders.push_back(reader);
        }
        if (name.rfind(ESE_READER_PREFIX, 0)) {
            eseReaders.push_back(reader);
        }
        if (name.rfind(SD_READER_PREFIX, 0)) {
            sdReaders.push_back(reader);
        }
    }

    EXPECT_EQ(result, true);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, OMAPISEServiceHalTest,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(
                             aidl::android::system::omapi::ISecureElementService::descriptor)),
                         android::hardware::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OMAPISEServiceHalTest);

}  // namespace
