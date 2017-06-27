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

#include <android/system/net/netd/1.0/INetdHwService.h>
#include <log/log.h>
#include <VtsHalHidlTargetTestBase.h>

#undef LOG_TAG
#define LOG_TAG "netd_hidl_test"

using ::android::system::net::netd::V1_0::INetdHwService;
using ::android::hardware::Return;
using ::android::sp;

class NetdHidlTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    virtual void SetUp() override {
        netd = ::testing::VtsHalHidlTargetTestBase::getService<INetdHwService>();
        ASSERT_NE(netd, nullptr) << "Could not get HIDL instance";
    }

    virtual void TearDown() override {}

    sp<INetdHwService> netd;
};

// positive test. Ensure netd creates an oem network and returns valid network_id, and destroys it.
TEST_F(NetdHidlTest, TestCreateAndDestroyOemNetworkOk) {
    ::uint64_t nId = 0;
    INetdHwService::StatusCode sc;
    auto cb = [&nId, &sc](::uint64_t networkId, INetdHwService::StatusCode status) {
        nId = networkId;
        sc = status;
    };

    Return<void> ret = netd->createOemNetwork(cb);
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE((sc == INetdHwService::StatusCode::OK) && (nId != 0));

    Return<INetdHwService::StatusCode> retStatus = netd->destroyOemNetwork(nId);
    ASSERT_TRUE(retStatus == INetdHwService::StatusCode::OK);
}

// negative test. Ensure destroy for invalid OEM network fails appropriately
TEST_F(NetdHidlTest, TestDestroyOemNetworkInvalid) {
     //NetworkController::MAX_OEM_ID++
     ::uint64_t nId = 51;

    Return<INetdHwService::StatusCode> retStatus = netd->destroyOemNetwork(nId);
    ASSERT_TRUE(retStatus == INetdHwService::StatusCode::INVALID_ARGUMENTS);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
