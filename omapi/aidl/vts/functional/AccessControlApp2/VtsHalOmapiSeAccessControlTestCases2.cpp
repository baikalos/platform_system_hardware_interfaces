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

#include <aidl/android/se/omapi/BnSecureElementListener.h>
#include <aidl/android/se/omapi/ISecureElementChannel.h>
#include <aidl/android/se/omapi/ISecureElementListener.h>
#include <aidl/android/se/omapi/ISecureElementReader.h>
#include <aidl/android/se/omapi/ISecureElementService.h>
#include <aidl/android/se/omapi/ISecureElementSession.h>
#include <aidl/android/se/omapi/SecureElementErrorCode.h>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
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

class OMAPISEAccessControlTest2 : public TestWithParam<std::string> {
   protected:
    class SEListener : public ::aidl::android::se::omapi::BnSecureElementListener {};

    void loadVendorStableReaders() {
        std::vector<std::string> readers = {};

        if (omapiSecureService() != NULL) {
            auto status = omapiSecureService()->getReaders(&readers);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            for (auto readerName : readers) {
                std::shared_ptr<::aidl::android::se::omapi::ISecureElementReader> reader;
                status = omapiSecureService()->getReader(readerName, &reader);
                ASSERT_TRUE(status.isOk()) << status.getMessage();

                mVSReaders[readerName] = reader;
            }
        }
    }

    void testSelectableAid(std::vector<uint8_t> aid) {
        std::shared_ptr<aidl::android::se::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::se::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEAccessControlTest2::SEListener>();

        if (mVSReaders.size() > 0) {
            for (const auto& [name, reader] : mVSReaders) {
                std::vector<uint8_t> selectResponse = {};
                ASSERT_NE(reader, nullptr) << "reader is null";

                bool status = false;
                auto res = reader->isSecureElementPresent(&status);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_TRUE(status);

                res = reader->openSession(&session);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(session, nullptr) << "Could not open session";

                res = session->openLogicalChannel(aid, 0x00, mSEListener, &channel);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(channel, nullptr) << "Could not open channel";

                res = channel->getSelectResponse(&selectResponse);
                ASSERT_TRUE(res.isOk()) << "failed to get Select Response";
                ASSERT_GE(selectResponse.size(), 2);

                if (channel != nullptr) channel->close();
                if (session != nullptr) session->close();

                ASSERT_EQ((selectResponse[selectResponse.size() - 1] & 0xFF), (0x00));
                ASSERT_EQ((selectResponse[selectResponse.size() - 2] & 0xFF), (0x90));
                ASSERT_TRUE(verifyBerTlvData(selectResponse)) << "Select Response is not complete";
            }
        }
    }

    void testUnauthorisedAid(std::vector<uint8_t> aid) {
        std::shared_ptr<aidl::android::se::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::se::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEAccessControlTest2::SEListener>();

        if (mVSReaders.size() > 0) {
            for (const auto& [name, reader] : mVSReaders) {
                ASSERT_NE(reader, nullptr) << "reader is null";

                bool status = false;
                auto res = reader->isSecureElementPresent(&status);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_TRUE(status);

                res = reader->openSession(&session);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(session, nullptr) << "Could not open session";

                res = session->openLogicalChannel(aid, 0x00, mSEListener, &channel);

                if (channel != nullptr) channel->close();
                if (session != nullptr) session->close();

                if (!res.isOk()) {
                    ASSERT_EQ(res.getExceptionCode(), EX_SECURITY);
                    ASSERT_FALSE(res.isOk()) << "expected failed status for this test";
                }
            }
        }
    }

    /**
     * Verifies TLV data
     *
     * @return true if the data is tlv formatted, false otherwise
     */
    bool verifyBerTlvData(std::vector<uint8_t> tlv) {
        if (tlv.size() == 0) {
            LOG(ERROR) << "Invalid tlv, null";
            return false;
        }
        int i = 0;
        if ((tlv[i++] & 0x1F) == 0x1F) {
            // extra byte for TAG field
            i++;
        }

        int len = tlv[i++] & 0xFF;
        if (len > 127) {
            // more than 1 byte for length
            int bytesLength = len - 128;
            len = 0;
            for (int j = bytesLength; j > 0; j--) {
                len += (len << 8) + (tlv[i++] & 0xFF);
            }
        }
        // Additional 2 bytes for the SW
        return (tlv.size() == (i + len + 2));
    }

    void testTransmitAPDU(std::vector<uint8_t> aid, std::vector<uint8_t> apdu) {
        std::shared_ptr<aidl::android::se::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::se::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEAccessControlTest2::SEListener>();

        if (mVSReaders.size() > 0) {
            for (const auto& [name, reader] : mVSReaders) {
                ASSERT_NE(reader, nullptr) << "reader is null";
                bool status = false;
                std::vector<uint8_t> selectResponse = {};
                std::vector<uint8_t> transmitResponse = {};
                auto res = reader->isSecureElementPresent(&status);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_TRUE(status);

                res = reader->openSession(&session);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(session, nullptr) << "Could not open session";

                res = session->openLogicalChannel(aid, 0x00, mSEListener, &channel);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(channel, nullptr) << "Could not open channel";

                res = channel->getSelectResponse(&selectResponse);
                ASSERT_TRUE(res.isOk()) << "failed to get Select Response";
                ASSERT_GE(selectResponse.size(), 2);
                ASSERT_EQ((selectResponse[selectResponse.size() - 1] & 0xFF), (0x00));
                ASSERT_EQ((selectResponse[selectResponse.size() - 2] & 0xFF), (0x90));
                ASSERT_TRUE(verifyBerTlvData(selectResponse)) << "Select Response is not complete";

                res = channel->transmit(apdu, &transmitResponse);
                LOG(INFO) << "STATUS OF TRNSMIT: " << res.getExceptionCode()
                          << " Message: " << res.getMessage();
                if (channel != nullptr) channel->close();
                if (session != nullptr) session->close();
                ASSERT_TRUE(res.isOk()) << "failed to transmit";
            }
        }
    }

    void testUnauthorisedAPDU(std::vector<uint8_t> aid, std::vector<uint8_t> apdu) {
        std::shared_ptr<aidl::android::se::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::se::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEAccessControlTest2::SEListener>();

        if (mVSReaders.size() > 0) {
            for (const auto& [name, reader] : mVSReaders) {
                ASSERT_NE(reader, nullptr) << "reader is null";
                bool status = false;
                std::vector<uint8_t> selectResponse = {};
                std::vector<uint8_t> transmitResponse = {};
                auto res = reader->isSecureElementPresent(&status);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_TRUE(status);

                res = reader->openSession(&session);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(session, nullptr) << "Could not open session";

                res = session->openLogicalChannel(aid, 0x00, mSEListener, &channel);
                ASSERT_TRUE(res.isOk()) << res.getMessage();
                ASSERT_NE(channel, nullptr) << "Could not open channel";

                res = channel->getSelectResponse(&selectResponse);
                ASSERT_TRUE(res.isOk()) << "failed to get Select Response";
                ASSERT_GE(selectResponse.size(), 2);
                ASSERT_EQ((selectResponse[selectResponse.size() - 1] & 0xFF), (0x00));
                ASSERT_EQ((selectResponse[selectResponse.size() - 2] & 0xFF), (0x90));
                ASSERT_TRUE(verifyBerTlvData(selectResponse)) << "Select Response is not complete";

                res = channel->transmit(apdu, &transmitResponse);
                LOG(INFO) << "STATUS OF TRNSMIT: " << res.getExceptionCode()
                          << " Message: " << res.getMessage();

                if (channel != nullptr) channel->close();
                if (session != nullptr) session->close();
                if (!res.isOk()) {
                    ASSERT_EQ(res.getExceptionCode(), EX_SECURITY);
                    ASSERT_FALSE(res.isOk()) << "expected failed status for this test";
                }
            }
        }
    }

    bool supportOMAPIReaders() {
        return (deviceSupportsFeature(FEATURE_SE_OMAPI_UICC.c_str()) ||
                deviceSupportsFeature(FEATURE_SE_OMAPI_ESE.c_str()) ||
                deviceSupportsFeature(FEATURE_SE_OMAPI_SD.c_str()));
    }

    void getFirstApiLevel(int32_t* outApiLevel) {
        int32_t firstApiLevel = property_get_int32(FEATURE_SE_API_LEVEL.c_str(), /*default*/ -1);
        if (firstApiLevel < 0) {
            firstApiLevel = property_get_int32(FEATURE_SE_SDK_VERSION.c_str(), /*default*/ -1);
        }
        ASSERT_GT(firstApiLevel, 0);  // first_api_level must exist
        *outApiLevel = firstApiLevel;
        return;
    }

    bool supportsHardware() {
        bool lowRamDevice = property_get_bool(FEATURE_SE_LOW_RAM.c_str(), true);
        return !lowRamDevice || deviceSupportsFeature(FEATURE_SE_HARDWARE_WATCH.c_str()) ||
               deviceSupportsFeature(FEATURE_SE_OMAPI_SERVICE.c_str());  // android.se.omapi
    }

    void SetUp() override {
        ASSERT_TRUE(supportsHardware());
        int32_t apiLevel;
        getFirstApiLevel(&apiLevel);
        ASSERT_TRUE(apiLevel > 27);
        ASSERT_TRUE(supportOMAPIReaders());
        ::ndk::SpAIBinder ks2Binder(AServiceManager_getService(omapiServiceName));
        omapiSeService = aidl::android::se::omapi::ISecureElementService::fromBinder(ks2Binder);
        ASSERT_TRUE(omapiSeService);

        loadVendorStableReaders();
    }

    void TearDown() override {
        if (omapiSeService != nullptr) {
            if (mVSReaders.size() > 0) {
                for (const auto& [name, reader] : mVSReaders) {
                    reader->closeSessions();
                }
            }
        }
    }

    std::shared_ptr<aidl::android::se::omapi::ISecureElementService> omapiSecureService() {
        return omapiSeService;
    }

    static inline std::string const UICC_READER_PREFIX = "SIM";
    static inline std::string const ESE_READER_PREFIX = "eSE";
    static inline std::string const SD_READER_PREFIX = "SD";
    static inline std::string const FEATURE_SE_OMAPI_UICC = "android.hardware.se.omapi.uicc";
    static inline std::string const FEATURE_SE_OMAPI_ESE = "android.hardware.se.omapi.ese";
    static inline std::string const FEATURE_SE_OMAPI_SD = "android.hardware.se.omapi.sd";
    static inline std::string const FEATURE_SE_LOW_RAM = "ro.config.low_ram";
    static inline std::string const FEATURE_SE_HARDWARE_WATCH = "android.hardware.type.watch";
    static inline std::string const FEATURE_SE_OMAPI_SERVICE = "com.android.se";
    static inline std::string const FEATURE_SE_SDK_VERSION = "ro.build.version.sdk";
    static inline std::string const FEATURE_SE_API_LEVEL = "ro.product.first_api_level";

    constexpr static const char omapiServiceName[] =
        "android.se.omapi.ISecureElementService/default";

    std::vector<uint8_t> AID_40 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x40};
    std::vector<uint8_t> AID_41 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x41};
    std::vector<uint8_t> AID_42 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x42};
    std::vector<uint8_t> AID_43 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x43};
    std::vector<uint8_t> AID_44 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x44};
    std::vector<uint8_t> AID_45 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x45};
    std::vector<uint8_t> AID_46 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x46};
    std::vector<uint8_t> AID_47 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x47};
    std::vector<uint8_t> AID_48 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x48};
    std::vector<uint8_t> AID_49 = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x49};
    std::vector<uint8_t> AID_4A = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4A};
    std::vector<uint8_t> AID_4B = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4B};
    std::vector<uint8_t> AID_4C = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4C};
    std::vector<uint8_t> AID_4D = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4D};
    std::vector<uint8_t> AID_4E = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4E};
    std::vector<uint8_t> AID_4F = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                   0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x4F};

    std::vector<std::vector<uint8_t>> AUTHORIZED_AID = {AID_40, AID_41, AID_43, AID_45, AID_46};
    std::vector<std::vector<uint8_t>> UNAUTHORIZED_AID = {
        AID_42, AID_44, AID_47, AID_48, AID_49, AID_4A, AID_4B, AID_4C, AID_4D, AID_4E, AID_4F};

    /* Authorized APDU for AID_40 */
    std::vector<std::vector<uint8_t>> AUTHORIZED_APDU_AID_40 = {
        {0x00, 0x06, 0x00, 0x00},
        {0xA0, 0x06, 0x00, 0x00},
    };
    /* Unauthorized APDU for AID_40 */
    std::vector<std::vector<uint8_t>> UNAUTHORIZED_APDU_AID_40 = {
        {0x00, 0x08, 0x00, 0x00, 0x00},
        {0x80, 0x06, 0x00, 0x00},
        {0xA0, 0x08, 0x00, 0x00, 0x00},
        {0x94, 0x06, 0x00, 0x00, 0x00},
    };

    /* Authorized APDU for AID_41 */
    std::vector<std::vector<uint8_t>> AUTHORIZED_APDU_AID_41 = {
        {0x94, 0x06, 0x00, 0x00},
        {0x94, 0x08, 0x00, 0x00, 0x00},
        {0x94, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
        {0x94, 0x0A, 0x00, 0x00, 0x01, 0xAA}};
    /* Unauthorized APDU for AID_41 */
    std::vector<std::vector<uint8_t>> UNAUTHORIZED_APDU_AID_41 = {
        {0x00, 0x06, 0x00, 0x00},
        {0x80, 0x06, 0x00, 0x00},
        {0xA0, 0x06, 0x00, 0x00},
        {0x00, 0x08, 0x00, 0x00, 0x00},
        {0x00, 0x0A, 0x00, 0x00, 0x01, 0xAA},
        {0x80, 0x0A, 0x00, 0x00, 0x01, 0xAA},
        {0xA0, 0x0A, 0x00, 0x00, 0x01, 0xAA},
        {0x80, 0x08, 0x00, 0x00, 0x00},
        {0xA0, 0x08, 0x00, 0x00, 0x00},
        {0x00, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
        {0x80, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
        {0xA0, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
    };

    std::shared_ptr<aidl::android::se::omapi::ISecureElementService> omapiSeService;

    std::map<std::string, std::shared_ptr<aidl::android::se::omapi::ISecureElementReader>>
        mVSReaders = {};
};

TEST_P(OMAPISEAccessControlTest2, TestAuthorizedAID) {
    for (auto aid : AUTHORIZED_AID) {
        testSelectableAid(aid);
    }
}

TEST_P(OMAPISEAccessControlTest2, TestUnauthorizedAID) {
    for (auto aid : UNAUTHORIZED_AID) {
        testUnauthorisedAid(aid);
    }
}

TEST_P(OMAPISEAccessControlTest2, TestAuthorizedAPDUAID40) {
    for (auto apdu : AUTHORIZED_APDU_AID_40) {
        testTransmitAPDU(AID_40, apdu);
    }
}

TEST_P(OMAPISEAccessControlTest2, TestUnauthorisedAPDUAID40) {
    for (auto apdu : UNAUTHORIZED_APDU_AID_40) {
        testUnauthorisedAPDU(AID_40, apdu);
    }
}

TEST_P(OMAPISEAccessControlTest2, TestAuthorizedAPDUAID41) {
    for (auto apdu : AUTHORIZED_APDU_AID_41) {
        testTransmitAPDU(AID_41, apdu);
    }
}

TEST_P(OMAPISEAccessControlTest2, TestUnauthorisedAPDUAID41) {
    for (auto apdu : UNAUTHORIZED_APDU_AID_41) {
        testUnauthorisedAPDU(AID_41, apdu);
    }
}

INSTANTIATE_TEST_SUITE_P(PerInstance, OMAPISEAccessControlTest2,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(
                             aidl::android::se::omapi::ISecureElementService::descriptor)),
                         android::hardware::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OMAPISEAccessControlTest2);

}  // namespace
