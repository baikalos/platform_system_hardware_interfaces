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

#include <VtsCoreUtil.h>
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
            auto status = omapiSecureService()->getReaders(&readers);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            LOG(INFO) << "OMAPI: Readers: ";
            for (auto readerName : readers) {
                LOG(INFO) << readerName;
                std::shared_ptr<::aidl::android::system::omapi::ISecureElementReader> reader;
                status = omapiSecureService()->getReader(readerName, &reader);
                ASSERT_TRUE(status.isOk()) << status.getMessage();

                mVSReaders[readerName] = reader;
            }
        }
    }

    std::shared_ptr<::aidl::android::system::omapi::ISecureElementReader> getUiccReader(
        int slotNumber) {
        if (slotNumber < 1) {
            LOG(ERROR) << "slotNumber should be larger than 0";
            return 0;
        }

        std::string readerName = UICC_READER_PREFIX + std::to_string(slotNumber);
        auto searchReader = mVSReaders.find(readerName);

        if (searchReader != mVSReaders.end()) {
            LOG(ERROR) << "Reader:" + readerName + " doesn't exist";
            return 0;
        }

        return searchReader->second;
    }

    bool supportOMAPIReaders() {
        return (deviceSupportsFeature(FEATURE_SE_OMAPI_UICC.c_str()) ||
                deviceSupportsFeature(FEATURE_SE_OMAPI_ESE.c_str()) ||
                deviceSupportsFeature(FEATURE_SE_OMAPI_SD.c_str()));
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
    static inline std::string const FEATURE_SE_OMAPI_UICC = "android.hardware.se.omapi.uicc";
    static inline std::string const FEATURE_SE_OMAPI_ESE = "android.hardware.se.omapi.ese";
    static inline std::string const FEATURE_SE_OMAPI_SD = "android.hardware.se.omapi.sd";

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
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> uiccReaders =
        {};
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> eseReaders =
        {};
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> sdReaders = {};

    for (const auto& [name, reader] : mVSReaders) {
        bool status = false;
        LOG(INFO) << "Name of the reader: " << name;

        if (reader) {
            auto res = reader->isSecureElementPresent(&status);
            ASSERT_TRUE(res.isOk()) << res.getMessage();
        }
        ASSERT_TRUE(status);

        if (!(name.rfind(UICC_READER_PREFIX, 0) || name.rfind(ESE_READER_PREFIX, 0) ||
              name.rfind(SD_READER_PREFIX))) {
            LOG(ERROR) << "Incorrect Reader name";
            FAIL();
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

    if (deviceSupportsFeature(FEATURE_SE_OMAPI_UICC.c_str())) {
        ASSERT_GE(uiccReaders.size(), 1);
        // Test API getUiccReader(int slotNumber)
        // The result should be the same as getReaders() with UICC reader prefix
        for (int i = 0; i < uiccReaders.size(); i++) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> uiccReader =
                getUiccReader((i + 1));
            if (uiccReader) {
                std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>>::
                    iterator it = std::find(uiccReaders.begin(), uiccReaders.end(), uiccReader);
                if (it == uiccReaders.end()) {
                    LOG(ERROR) << "Incorrect reader object - getUiccReader(" +
                                      std::to_string(i + 1) + ")";
                    FAIL();
                }
            } else {
                LOG(ERROR) << "Fail to get Reader object by calling getUiccReader(" +
                                  std::to_string(i + 1) + ")";
                FAIL();
            }
        }
    } else {
        ASSERT_TRUE(uiccReaders.size() == 0);
    }

    if (deviceSupportsFeature(FEATURE_SE_OMAPI_ESE.c_str())) {
        ASSERT_GE(eseReaders.size(), 1);
    } else {
        ASSERT_TRUE(eseReaders.size() == 0);
    }

    if (deviceSupportsFeature(FEATURE_SE_OMAPI_SD.c_str())) {
        ASSERT_GE(sdReaders.size(), 1);
    } else {
        ASSERT_TRUE(sdReaders.size() == 0);
    }
}

TEST_P(OMAPISEServiceHalTest, testATR) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> uiccReaders =
        {};
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            if (name.rfind(UICC_READER_PREFIX, 0)) {
                uiccReaders.push_back(reader);
            }
        }

        for (auto itReader = std::begin(uiccReaders); itReader != std::end(uiccReaders);
             ++itReader) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
            auto status = (*itReader)->openSession(&session);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
            if (!session) {
                LOG(ERROR) << "Could not open session";
                FAIL();
            }
            std::vector<uint8_t>* atr = NULL;
            status = session->getAtr(atr);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
            session->close();
            if (!atr) {
                LOG(ERROR) << "ATR is Null";
                FAIL();
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(PerInstance, OMAPISEServiceHalTest,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(
                             aidl::android::system::omapi::ISecureElementService::descriptor)),
                         android::hardware::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OMAPISEServiceHalTest);

}  // namespace
