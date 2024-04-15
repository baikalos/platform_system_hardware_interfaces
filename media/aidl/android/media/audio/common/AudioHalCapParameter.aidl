/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.media.audio.common;

/**
 * Defines the audio Cap Engine Parameters expected to be controlled by the configurable engine.
 * These parameters deal with:
 *    Volume Profile: for volume curve selection (e.g. dtmf follows call curves during call).
 *    Output/Input device selection for a given strategy based on:
 *        -the type (each device will be a bit in a bitfield, allowing to select multiple devices).
 *        -the address
 *
 * {@hide}
 */
@Backing(type="int") @JavaDerive(toString=true)
@VintfStability
enum AudioHalCapParameter {
    SYS_RESERVED_INVALID = -1,

    VOLUME_PROFILE = 0,

    COMMUNICATION = 1,
    AMBIENT = 2,
    BUILTIN_MIC = 3,
    BLUETOOTH_SCO_HEADSET = 4,
    WIRED_HEADSET = 5,
    HDMI = 6,
    TELEPHONY_RX = 7,
    BACK_MIC = 8,
    REMOTE_SUBMIX = 9,
    ANLG_DOCK_HEADSET = 10,
    DGTL_DOCK_HEADSET = 11,
    USB_ACCESSORY = 12,
    USB_DEVICE = 13,
    FM_TUNER = 14,
    TV_TUNER = 15,
    LINE = 16,
    SPDIF = 17,
    BLUETOOTH_A2DP = 18,
    LOOPBACK = 19,
    IP = 20,
    BUS = 21,
    PROXY = 22,
    USB_HEADSET = 23,
    BLUETOOTH_BLE = 24,
    HDMI_ARC = 25,
    ECHO_REFERENCE = 26,
    BLE_HEADSET = 27,
    STUB = 28,
    HDMI_EARC = 29,

    DEVICE_ADDRESS = 30,

    EARPIECE = 31,
    SPEAKER = 32,
    WIRED_HEADPHONE = 33,
    BLUETOOTH_SCO = 34,
    BLUETOOTH_SCO_CARKIT = 35,
    BLUETOOTH_A2DP_HEADPHONES = 36,
    BLUETOOTH_A2DP_SPEAKER = 37,
    TELEPHONY_TX = 38,
    FM = 39,
    AUX_LINE = 40,
    SPEAKER_SAFE = 41,
    HEARING_AID = 42,
    ECHO_CANCELLER = 43,
    BLE_SPEAKER = 44,
    BLE_BROADCAST = 45,
}
