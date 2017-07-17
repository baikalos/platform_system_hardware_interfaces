/*
 * Copyright 2017 The Android Open Source Project
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
#define LOG_TAG "netd_hidl_test"

#include <android/system/net/netd/1.0/INetdHwService.h>
#include <log/log.h>
#include <VtsHalHidlTargetTestBase.h>


using ::android::system::net::netd::V1_0::INetdHwService;
using ::android::hardware::Return;
using ::android::sp;

class NetdHidlTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        netd = ::testing::VtsHalHidlTargetTestBase::getService<INetdHwService>();
        ASSERT_NE(nullptr, netd.get()) << "Could not get HIDL instance";
    }

    sp<INetdHwService> netd;
};

// positive test. Ensure netd creates an oem network and returns valid netHandle, and destroys it.
TEST_F(NetdHidlTest, TestCreateAndDestroyOemNetworkOk) {
    auto cb = [this](uint64_t netHandle,
            uint64_t packetMark, INetdHwService::StatusCode status) {

        ASSERT_EQ(INetdHwService::StatusCode::OK, status);
        ASSERT_NE((uint64_t)0, netHandle);
        ASSERT_NE((uint64_t)0, packetMark);

        Return<INetdHwService::StatusCode> retStatus = netd->destroyOemNetwork(netHandle);
        ASSERT_EQ(INetdHwService::StatusCode::OK, retStatus);
    };

    Return<void> ret = netd->createOemNetwork(cb);
    ASSERT_TRUE(ret.isOk());
}

// negative test. Ensure destroy for invalid OEM network fails appropriately
TEST_F(NetdHidlTest, TestDestroyOemNetworkInvalid) {
    uint64_t nh = 0x6600FACADE;

    Return<INetdHwService::StatusCode> retStatus = netd->destroyOemNetwork(nh);
    ASSERT_EQ(INetdHwService::StatusCode::INVALID_ARGUMENTS, retStatus);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
