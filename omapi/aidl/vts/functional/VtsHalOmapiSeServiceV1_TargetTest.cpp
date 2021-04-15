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

#include <aidl/android/system/omapi/BnSecureElementListener.h>
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
    class SEListener : public ::aidl::android::system::omapi::BnSecureElementListener {
        // TODO do we need to override
    };

    void loadVendorStableReaders() {
        std::vector<std::string> readers = {};

        if (omapiSecureService() != NULL) {
            auto status = omapiSecureService()->getReaders(&readers);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            for (auto readerName : readers) {
                std::shared_ptr<::aidl::android::system::omapi::ISecureElementReader> reader;
                status = omapiSecureService()->getReader(readerName, &reader);
                ASSERT_TRUE(status.isOk()) << status.getMessage();

                mVSReaders[readerName] = reader;
            }
        }
    }

    void testSelectableAid(
        std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> reader,
        std::vector<uint8_t> aid, std::vector<uint8_t>& selectResponse) {
        std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();

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
    }

    void testNonSelectableAid(
        std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> reader,
        std::vector<uint8_t> aid) {
        std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();

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

        LOG(ERROR) << res.getMessage();
        ASSERT_FALSE(res.isOk()) << "expected to fail to open channel for this test";
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

    void internalTransmitApdu(
        std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> reader,
        std::vector<uint8_t> apdu, std::vector<uint8_t>& transmitResponse) {
        std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
        std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
        auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();
        std::vector<uint8_t> selectResponse = {};

        ASSERT_NE(reader, nullptr) << "reader is null";

        bool status = false;
        auto res = reader->isSecureElementPresent(&status);
        ASSERT_TRUE(res.isOk()) << res.getMessage();
        ASSERT_TRUE(status);

        res = reader->openSession(&session);
        ASSERT_TRUE(res.isOk()) << res.getMessage();
        ASSERT_NE(session, nullptr) << "Could not open session";

        res = session->openLogicalChannel(SELECTABLE_AID, 0x00, mSEListener, &channel);
        ASSERT_TRUE(res.isOk()) << res.getMessage();
        ASSERT_NE(channel, nullptr) << "Could not open channel";

        res = channel->getSelectResponse(&selectResponse);
        ASSERT_TRUE(res.isOk()) << "failed to get Select Response";
        ASSERT_GE(selectResponse.size(), 2);

        res = channel->transmit(apdu, &transmitResponse);
        if (channel != nullptr) channel->close();
        if (session != nullptr) session->close();
        LOG(INFO) << "STATUS OF TRNSMIT: " << res.getExceptionCode()
                  << " Message: " << res.getMessage();
        ASSERT_TRUE(res.isOk()) << "failed to transmit";
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

    static inline std::string const UICC_READER_PREFIX = "SIM";
    static inline std::string const ESE_READER_PREFIX = "eSE";
    static inline std::string const SD_READER_PREFIX = "SD";
    static inline std::string const FEATURE_SE_OMAPI_UICC = "android.hardware.se.omapi.uicc";
    static inline std::string const FEATURE_SE_OMAPI_ESE = "android.hardware.se.omapi.ese";
    static inline std::string const FEATURE_SE_OMAPI_SD = "android.hardware.se.omapi.sd";

    constexpr static const char omapiServiceName[] =
        "android.system.omapi.ISecureElementService/default";

    std::vector<uint8_t> SELECTABLE_AID = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                           0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x31};
    std::vector<uint8_t> LONG_SELECT_RESPONSE_AID = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41,
                                                     0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64,
                                                     0x43, 0x54, 0x53, 0x32};
    std::vector<uint8_t> NON_SELECTABLE_AID = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
                                               0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0xFF};

    std::vector<std::vector<uint8_t>> ILLEGAL_COMMANDS_TRANSMIT = {
        {0x00, 0x70, 0x00, 0x00},
        {0x00, 0x70, 0x80, 0x00},
        {0x00, 0xA4, 0x04, 0x04, 0x10, 0x4A, 0x53, 0x52, 0x31, 0x37, 0x37,
         0x54, 0x65, 0x73, 0x74, 0x65, 0x72, 0x20, 0x31, 0x2E, 0x30}};

    /* OMAPI APDU Test case 1 and 3 */
    std::vector<std::vector<uint8_t>> NO_DATA_APDU = {{0x00, 0x06, 0x00, 0x00},
                                                      {0x80, 0x06, 0x00, 0x00},
                                                      {0xA0, 0x06, 0x00, 0x00},
                                                      {0x94, 0x06, 0x00, 0x00},
                                                      {0x00, 0x0A, 0x00, 0x00, 0x01, 0xAA},
                                                      {0x80, 0x0A, 0x00, 0x00, 0x01, 0xAA},
                                                      {0xA0, 0x0A, 0x00, 0x00, 0x01, 0xAA},
                                                      {0x94, 0x0A, 0x00, 0x00, 0x01, 0xAA}};

    /* OMAPI APDU Test case 2 and 4 */
    std::vector<std::vector<uint8_t>> DATA_APDU = {{0x00, 0x08, 0x00, 0x00, 0x00},
                                                   {0x80, 0x08, 0x00, 0x00, 0x00},
                                                   {0xA0, 0x08, 0x00, 0x00, 0x00},
                                                   {0x94, 0x08, 0x00, 0x00, 0x00},
                                                   {0x00, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
                                                   {0x80, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
                                                   {0xA0, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00},
                                                   {0x94, 0x0C, 0x00, 0x00, 0x01, 0xAA, 0x00}};

    /* Case 2 APDU command expects the P2 received in the SELECT command as 1-byte outgoing data */
    std::vector<uint8_t> CHECK_SELECT_P2_APDU = {0x00, 0xF4, 0x00, 0x00, 0x00};

    /* OMAPI APDU Test case 1 and 3 */
    std::vector<std::vector<uint8_t>> SW_62xx_NO_DATA_APDU = {{0x00, 0xF3, 0x00, 0x06},
                                                              {0x00, 0xF3, 0x00, 0x0A, 0x01, 0xAA}};

    /* OMAPI APDU Test case 2 and 4 */
    std::vector<uint8_t> SW_62xx_DATA_APDU = {0x00, 0xF3, 0x00, 0x08, 0x00};
    std::vector<uint8_t> SW_62xx_VALIDATE_DATA_APDU = {0x00, 0xF3, 0x00, 0x0C, 0x01, 0xAA, 0x00};
    std::vector<std::vector<uint8_t>> SW_62xx = {
        {0x62, 0x00}, {0x62, 0x81}, {0x62, 0x82}, {0x62, 0x83}, {0x62, 0x85}, {0x62, 0xF1},
        {0x62, 0xF2}, {0x63, 0xF1}, {0x63, 0xF2}, {0x63, 0xC2}, {0x62, 0x02}, {0x62, 0x80},
        {0x62, 0x84}, {0x62, 0x86}, {0x63, 0x00}, {0x63, 0x81}};

    std::vector<std::vector<uint8_t>> SEGMENTED_RESP_APDU = {
        // Get response Case2 61FF+61XX with answer length (P1P2) of 0x0800, 2048 bytes
        {0x00, 0xC2, 0x08, 0x00, 0x00},
        // Get response Case4 61FF+61XX with answer length (P1P2) of 0x0800, 2048 bytes
        {0x00, 0xC4, 0x08, 0x00, 0x02, 0x12, 0x34, 0x00},
        // Get response Case2 6100+61XX with answer length (P1P2) of 0x0800, 2048 bytes
        {0x00, 0xC6, 0x08, 0x00, 0x00},
        // Get response Case4 6100+61XX with answer length (P1P2) of 0x0800, 2048 bytes
        {0x00, 0xC8, 0x08, 0x00, 0x02, 0x12, 0x34, 0x00},
        // Test device buffer capacity 7FFF data
        {0x00, 0xC2, 0x7F, 0xFF, 0x00},
        // Get response 6CFF+61XX with answer length (P1P2) of 0x0800, 2048 bytes
        {0x00, 0xCF, 0x08, 0x00, 0x00},
        // Get response with another CLA  with answer length (P1P2) of 0x0800, 2048 bytes
        {0x94, 0xC2, 0x08, 0x00, 0x00}};
    long SERVICE_CONNECTION_TIME_OUT = 3000;

    std::shared_ptr<aidl::android::system::omapi::ISecureElementService> omapiSeService;

    std::map<std::string, std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>>
        mVSReaders = {};
};

/** Tests getReaders API */
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

        if (!(name.rfind(UICC_READER_PREFIX, 0) == 0 || name.rfind(ESE_READER_PREFIX, 0) == 0 ||
              name.rfind(SD_READER_PREFIX) == 0)) {
            LOG(ERROR) << "Incorrect Reader name";
            FAIL();
        }

        if (name.rfind(UICC_READER_PREFIX, 0) == 0) {
            uiccReaders.push_back(reader);
        } else if (name.rfind(ESE_READER_PREFIX, 0) == 0) {
            eseReaders.push_back(reader);
        } else if (name.rfind(SD_READER_PREFIX, 0) == 0) {
            sdReaders.push_back(reader);
        } else {
            LOG(INFO) << "Reader not supported: " << name;
            FAIL();
        }
    }

    if (deviceSupportsFeature(FEATURE_SE_OMAPI_UICC.c_str())) {
        ASSERT_GE(uiccReaders.size(), 1);
        // Test API getReader(readerName)
        // The result should be the same as getReaders() with UICC reader prefix
        for (int slotNumber = 1; slotNumber <= uiccReaders.size(); slotNumber++) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementReader> uiccReader;
            std::string readerName = UICC_READER_PREFIX + std::to_string(slotNumber);
            auto status = omapiSecureService()->getReader(readerName, &uiccReader);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
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

/** Tests getATR API */
TEST_P(OMAPISEServiceHalTest, TestATR) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    std::vector<std::shared_ptr<aidl::android::system::omapi::ISecureElementReader>> uiccReaders =
        {};
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            if (name.rfind(UICC_READER_PREFIX, 0) == 0) {
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
            std::vector<uint8_t> atr = {};
            status = session->getAtr(&atr);
            ASSERT_TRUE(status.isOk());
            session->close();
            if (atr.size() == 0) {
                LOG(ERROR) << "Failed to get attributes";
                FAIL();
            }
        }
    }
}

/** Tests OpenBasicChannel API when aid is null */
TEST_P(OMAPISEServiceHalTest, TestOpenBasicChannelNullAid) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    std::vector<uint8_t> aid = {};
    auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();

    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
            std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
            bool result = false;

            auto status = reader->openSession(&session);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
            if (!session) {
                LOG(ERROR) << "Could not open session";
                FAIL();
            }

            status = session->openBasicChannel(aid, 0x00, mSEListener, &channel);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            if (channel != nullptr) channel->close();
            if (session != nullptr) session->close();

            if (name.rfind(UICC_READER_PREFIX, 0) == 0) {
                if (channel != nullptr) {
                    status = channel->isBasicChannel(&result);
                    ASSERT_FALSE(status.isOk()) << "Basic channel on UICC can be opened";
                }
            } else {
                if (channel != nullptr) {
                    status = channel->isBasicChannel(&result);
                    ASSERT_TRUE(status.isOk()) << "Basic Channel cannot be opened";
                }
            }
        }
    }
}

/** Tests OpenBasicChannel API when aid is provided */
TEST_P(OMAPISEServiceHalTest, TestOpenBasicChannelNonNullAid) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();

    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
            std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
            bool result = false;

            auto status = reader->openSession(&session);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
            if (!session) {
                LOG(ERROR) << "Could not open session";
                FAIL();
            }

            status = session->openBasicChannel(SELECTABLE_AID, 0x00, mSEListener, &channel);
            ASSERT_TRUE(status.isOk()) << status.getMessage();

            if (channel != nullptr) channel->close();
            if (session != nullptr) session->close();

            if (name.rfind(UICC_READER_PREFIX, 0) == 0) {
                if (channel != nullptr) {
                    status = channel->isBasicChannel(&result);
                    ASSERT_FALSE(status.isOk()) << "Basic channel on UICC can be opened";
                }
            } else {
                if (channel != nullptr) {
                    status = channel->isBasicChannel(&result);
                    ASSERT_TRUE(status.isOk()) << "Basic Channel cannot be opened";
                }
            }
        }
    }
}

/** Tests Select API */
TEST_P(OMAPISEServiceHalTest, TestSelectableAid) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            std::vector<uint8_t> selectResponse = {};
            testSelectableAid(reader, SELECTABLE_AID, selectResponse);
        }
    }
}

/** Tests Select API */
TEST_P(OMAPISEServiceHalTest, TestLongSelectResponse) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            std::vector<uint8_t> selectResponse = {};
            testSelectableAid(reader, LONG_SELECT_RESPONSE_AID, selectResponse);
            ASSERT_TRUE(verifyBerTlvData(selectResponse)) << "Select Response is not complete";
        }
    }
}

/** Test to fail open channel with wrong aid */
TEST_P(OMAPISEServiceHalTest, TestWrongAid) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            testNonSelectableAid(reader, NON_SELECTABLE_AID);
        }
    }
}

/** Tests with invalid cmds in Transmit */
TEST_P(OMAPISEServiceHalTest, TestSecurityExceptionInTransmit) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            std::shared_ptr<aidl::android::system::omapi::ISecureElementSession> session;
            std::shared_ptr<aidl::android::system::omapi::ISecureElementChannel> channel;
            auto mSEListener = std::make_shared<::OMAPISEServiceHalTest::SEListener>();
            std::vector<uint8_t> selectResponse = {};

            ASSERT_NE(reader, nullptr) << "reader is null";

            bool status = false;
            auto res = reader->isSecureElementPresent(&status);
            ASSERT_TRUE(res.isOk()) << res.getMessage();
            ASSERT_TRUE(status);

            res = reader->openSession(&session);
            ASSERT_TRUE(res.isOk()) << res.getMessage();
            ASSERT_NE(session, nullptr) << "Could not open session";

            res = session->openLogicalChannel(SELECTABLE_AID, 0x00, mSEListener, &channel);
            ASSERT_TRUE(res.isOk()) << res.getMessage();
            ASSERT_NE(channel, nullptr) << "Could not open channel";

            res = channel->getSelectResponse(&selectResponse);
            ASSERT_TRUE(res.isOk()) << "failed to get Select Response";
            ASSERT_GE(selectResponse.size(), 2);

            ASSERT_EQ((selectResponse[selectResponse.size() - 1] & 0xFF), (0x00));
            ASSERT_EQ((selectResponse[selectResponse.size() - 2] & 0xFF), (0x90));

            for (auto cmd : ILLEGAL_COMMANDS_TRANSMIT) {
                std::vector<uint8_t> response = {};
                res = channel->transmit(cmd, &response);
                if (!res.isOk()) {
                    ASSERT_EQ(res.getExceptionCode(), EX_SECURITY);
                    ASSERT_FALSE(res.isOk()) << "expected failed status for this test";
                }
            }
            if (channel != nullptr) channel->close();
            if (session != nullptr) session->close();
        }
    }
}

/**
 * Tests Transmit API for all readers.
 *
 * Checks the return status and verifies the size of the
 * response.
 */
TEST_P(OMAPISEServiceHalTest, TestTransmitApdu) {
    ASSERT_TRUE(supportOMAPIReaders() == true);
    if (mVSReaders.size() > 0) {
        for (const auto& [name, reader] : mVSReaders) {
            for (auto apdu : NO_DATA_APDU) {
                std::vector<uint8_t> response = {};
                internalTransmitApdu(reader, apdu, response);
                ASSERT_GE(response.size(), 2);
                ASSERT_EQ((response[response.size() - 1] & 0xFF), (0x00));
                ASSERT_EQ((response[response.size() - 2] & 0xFF), (0x90));
            }

            for (auto apdu : DATA_APDU) {
                std::vector<uint8_t> response = {};
                internalTransmitApdu(reader, apdu, response);
                /* 256 byte data and 2 bytes of status word */
                ASSERT_GE(response.size(), 258);
                ASSERT_EQ((response[response.size() - 1] & 0xFF), (0x00));
                ASSERT_EQ((response[response.size() - 2] & 0xFF), (0x90));
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
