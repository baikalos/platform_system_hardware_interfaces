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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.media.audio.common;
/* @hide */
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum AudioHalCapParameter {
  SYS_RESERVED_INVALID = (-1) /* -1 */,
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
