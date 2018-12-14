/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <android-base/logging.h>

#include <VtsHalHidlTargetTestBase.h>
#include <private/android_filesystem_config.h>
#include <utils/String16.h>
#include <wifikeystorehal/keystore.h>

using namespace std;
using namespace android;
using namespace android::binder;
using namespace android::security::keystore;
using namespace android::security::keymaster;
using namespace android::system::wifi::keystore::V1_0;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}

namespace {

// The fixture for testing the Wifi Keystore HAL
class WifiKeystoreHalTest : public ::testing::Test {
   protected:
    void SetUp() override { keystore = implementation::HIDL_FETCH_IKeystore(nullptr); }

    IKeystore* keystore = nullptr;
};

TEST_F(WifiKeystoreHalTest, GetBlob) {
    IKeystore::KeystoreStatusCode statusCode;

    auto callback = [&statusCode](IKeystore::KeystoreStatusCode status,
                                  const ::android::hardware::hidl_vec<uint8_t>& /*value*/) {
        statusCode = status;
        return;
    };

    keystore->getBlob("", callback);
    EXPECT_EQ(IKeystore::KeystoreStatusCode::ERROR_UNKNOWN, statusCode);
}

TEST_F(WifiKeystoreHalTest, GetPublicKey) {
    IKeystore::KeystoreStatusCode statusCode;

    auto callback = [&statusCode](IKeystore::KeystoreStatusCode status,
                                  const ::android::hardware::hidl_vec<uint8_t>& /*value*/) {
        statusCode = status;
        return;
    };

    keystore->getPublicKey("", callback);
    EXPECT_EQ(IKeystore::KeystoreStatusCode::ERROR_UNKNOWN, statusCode);
}

TEST_F(WifiKeystoreHalTest, GetKeyAlgoritmFromKeyCharacteristics) {}

TEST_F(WifiKeystoreHalTest, Sign) {
    IKeystore::KeystoreStatusCode statusCode;

    auto callback = [&statusCode](IKeystore::KeystoreStatusCode status,
                                  const ::android::hardware::hidl_vec<uint8_t>& /*value*/) {
        statusCode = status;
        return;
    };

    ::android::hardware::hidl_vec<uint8_t> dataToSign;

    keystore->sign("", dataToSign, callback);
    EXPECT_EQ(IKeystore::KeystoreStatusCode::ERROR_UNKNOWN, statusCode);
}

}  // namespace
