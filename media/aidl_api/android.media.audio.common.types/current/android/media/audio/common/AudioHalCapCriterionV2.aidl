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
@VintfStability
union AudioHalCapCriterionV2 {
  android.media.audio.common.AudioHalCapCriterionV2.AvailableDevices availableInputDevices;
  android.media.audio.common.AudioHalCapCriterionV2.AvailableDevices availableOutputDevices;
  android.media.audio.common.AudioHalCapCriterionV2.AvailableDevicesAddresses availableInputDevicesAddresses;
  android.media.audio.common.AudioHalCapCriterionV2.AvailableDevicesAddresses availableOutputDevicesAddresses;
  android.media.audio.common.AudioHalCapCriterionV2.TelephonyMode telephonyMode;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForCommunication;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForMedia;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForRecord;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForDock;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForSystem;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForHdmiSystemAudio;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForEncodedSurround;
  android.media.audio.common.AudioHalCapCriterionV2.ForceConfigForUse forceUseForVibrateRinging;
  android.media.audio.common.AudioHalCapCriterionV2.Type type;
  @VintfStability
  enum LogicalDisjunction {
    EXLUSIVE = 0,
    INCLUSIVE,
  }
  @VintfStability
  parcelable ForceConfigForUse {
    android.media.audio.common.AudioPolicyForceUse forceUse = android.media.audio.common.AudioPolicyForceUse.MEDIA;
    @nullable android.media.audio.common.AudioPolicyForcedConfig[] values;
    android.media.audio.common.AudioPolicyForcedConfig defaultValue = android.media.audio.common.AudioPolicyForcedConfig.NONE;
    android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction logic = android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction.EXLUSIVE;
  }
  @VintfStability
  parcelable TelephonyMode {
    @nullable android.media.audio.common.AudioMode[] values;
    android.media.audio.common.AudioMode defaultValue = android.media.audio.common.AudioMode.NORMAL;
    android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction logic = android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction.EXLUSIVE;
  }
  @VintfStability
  parcelable AvailableDevices {
    @nullable android.media.audio.common.AudioDeviceDescription[] values;
    android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction logic = android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction.INCLUSIVE;
  }
  @VintfStability
  parcelable AvailableDevicesAddresses {
    @nullable android.media.audio.common.AudioDeviceAddress[] values;
    android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction logic = android.media.audio.common.AudioHalCapCriterionV2.LogicalDisjunction.INCLUSIVE;
  }
  @VintfStability
  union Type {
    android.media.audio.common.AudioDeviceDescription availableDevicesType;
    android.media.audio.common.AudioDeviceAddress availableDevicesAddressesType;
    android.media.audio.common.AudioMode telephonyModeType;
    android.media.audio.common.AudioPolicyForcedConfig forcedConfigType;
  }
}
