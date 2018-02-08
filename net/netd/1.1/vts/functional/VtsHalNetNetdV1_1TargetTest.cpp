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

#include <VtsHalHidlTargetTestBase.h>
#include <android/system/net/netd/1.1/INetd.h>
#include <log/log.h>

#include "VtsHalNetNetdTestUtils.h"
#include "tun_interface.h"

using ::android::system::net::netd::V1_1::INetd;
using ::android::hardware::Return;
using ::android::sp;
using android::net::TunInterface;

class NetdHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        netd = ::testing::VtsHalHidlTargetTestBase::getService<INetd>();
        ASSERT_NE(nullptr, netd.get()) << "Could not get HIDL instance";
    }

    sp<INetd> netd;
    static TunInterface sTun;

    static void SetUpTestCase() {
        ASSERT_EQ(0, sTun.init());
        ASSERT_LE(sTun.name().size(), static_cast<size_t>(IFNAMSIZ));
    }

    static void TearDownTestCase() { sTun.destroy(); }
};

TunInterface NetdHidlTest::sTun;

TEST_F(NetdHidlTest, TestOemNetworkInterfaces) {
    net_handle_t netHandle;
    uint32_t packetMark;
    INetd::StatusCode status;

    Return<void> ret = netd->createOemNetwork([&](net_handle_t n, uint32_t p, INetd::StatusCode s) {
        status = s;
        netHandle = n;
        packetMark = p;
    });

    ASSERT_TRUE(ret.isOk());
    ASSERT_EQ(INetd::StatusCode::OK, status);
    ASSERT_NE(NETWORK_UNSPECIFIED, netHandle);
    ASSERT_NE((uint32_t)0, packetMark);

    // HACK: mark out permissions bits.
    packetMark &= 0xffff;

    EXPECT_EQ(0, checkNetworkExists(netHandle));
    EXPECT_EQ(0, countRulesForFwmark(packetMark));

    // Adding an interface creates the routing rules.
    Return<INetd::StatusCode> retStatus = netd->addInterfaceToOemNetwork(netHandle, sTun.name());
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(0, checkNetworkExists(netHandle));
    EXPECT_EQ(2, countRulesForFwmark(packetMark));

    // Adding an interface again silently succeeds.
    retStatus = netd->addInterfaceToOemNetwork(netHandle, sTun.name());
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(0, checkNetworkExists(netHandle));
    EXPECT_EQ(2, countRulesForFwmark(packetMark));

    // More than one network can be created.
    net_handle_t netHandle2;
    uint32_t packetMark2;

    ret = netd->createOemNetwork([&](net_handle_t n, uint32_t p, INetd::StatusCode) {
        netHandle2 = n;
        packetMark2 = p;
    });
    EXPECT_TRUE(ret.isOk());
    EXPECT_NE(netHandle, netHandle2);
    EXPECT_NE(packetMark, packetMark2);
    EXPECT_EQ(0, checkNetworkExists(netHandle2));
    EXPECT_EQ(0, countRulesForFwmark(packetMark2));

    // An interface can only be in one network.
    retStatus = netd->addInterfaceToOemNetwork(netHandle2, sTun.name());
    EXPECT_TRUE(retStatus.isOk());
    EXPECT_EQ(INetd::StatusCode::UNKNOWN_ERROR, retStatus);

    // Removing the interface removes the rules.
    retStatus = netd->removeInterfaceFromOemNetwork(netHandle, sTun.name());
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(0, countRulesForFwmark(packetMark));

    // When a network is removed the interfaces are deleted.
    retStatus = netd->addInterfaceToOemNetwork(netHandle, sTun.name());
    EXPECT_TRUE(ret.isOk());
    EXPECT_EQ(2, countRulesForFwmark(packetMark));

    retStatus = netd->destroyOemNetwork(netHandle);
    EXPECT_TRUE(retStatus.isOk());
    EXPECT_EQ(INetd::StatusCode::OK, retStatus);
    EXPECT_EQ(-ENONET, checkNetworkExists(netHandle));
    EXPECT_EQ(0, countRulesForFwmark(packetMark));

    // Clean up.
    retStatus = netd->destroyOemNetwork(netHandle2);
    EXPECT_TRUE(retStatus.isOk());
    EXPECT_EQ(INetd::StatusCode::OK, retStatus);
    EXPECT_EQ(-ENONET, checkNetworkExists(netHandle2));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test result with status=%d", status);
    return status;
}
