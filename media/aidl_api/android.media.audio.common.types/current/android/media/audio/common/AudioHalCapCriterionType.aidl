/*
 * Copyright (C) 2022 The Android Open Source Project
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
@JavaDerive(equals=true, toString=true) @SuppressWarnings(value={"redundant-name"}) @VintfStability
parcelable AudioHalCapCriterionType {
  @utf8InCpp String name;
  boolean isInclusive;
  @utf8InCpp String[] values;
  @nullable long[] numericalValue;
  @nullable android.media.audio.common.AudioDeviceDescription[] androidMappedValue;
  const @utf8InCpp String SUFFIX = "Type";
  const @utf8InCpp String AVAILABLE_INPUT_DEVICES_TYPE = (android.media.audio.common.AudioHalCapCriterion.AVAILABLE_INPUT_DEVICES + SUFFIX) /* "AvailableInputDevicesType" */;
  const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_TYPE = (android.media.audio.common.AudioHalCapCriterion.AVAILABLE_OUTPUT_DEVICES + SUFFIX) /* "AvailableOutputDevicesType" */;
  const @utf8InCpp String AVAILABLE_INPUT_DEVICES_ADDRESSES_TYPE = (android.media.audio.common.AudioHalCapCriterion.AVAILABLE_INPUT_DEVICES_ADDRESSES + SUFFIX) /* "AvailableInputDevicesAddressesType" */;
  const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_ADDRESSES_TYPE = (android.media.audio.common.AudioHalCapCriterion.AVAILABLE_OUTPUT_DEVICES_ADDRESSES + SUFFIX) /* "AvailableOutputDevicesAddressesType" */;
  const @utf8InCpp String TELEPHONY_MODE_TYPE = (android.media.audio.common.AudioHalCapCriterion.TELEPHONY_MODE + SUFFIX) /* "TelephonyModeType" */;
  const @utf8InCpp String FORCE_USE_FOR_COMMUNICATION_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_COMMUNICATION + SUFFIX) /* "ForceUseForCommunicationType" */;
  const @utf8InCpp String FORCE_USE_FOR_MEDIA_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_MEDIA + SUFFIX) /* "ForceUseForMediaType" */;
  const @utf8InCpp String FORCE_USE_FOR_RECORD_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_RECORD + SUFFIX) /* "ForceUseForRecordType" */;
  const @utf8InCpp String FORCE_USE_FOR_DOCK_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_DOCK + SUFFIX) /* "ForceUseForDockType" */;
  const @utf8InCpp String FORCE_USE_FOR_SYSTEM_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_SYSTEM + SUFFIX) /* "ForceUseForSystemType" */;
  const @utf8InCpp String FORCE_USE_FOR_HDMI_SYSTEM_AUDIO_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_HDMI_SYSTEM_AUDIO + SUFFIX) /* "ForceUseForHdmiSystemAudioType" */;
  const @utf8InCpp String FORCE_USE_FOR_ENCODED_SURROUND_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_ENCODED_SURROUND + SUFFIX) /* "ForceUseForEncodedSurroundType" */;
  const @utf8InCpp String FORCE_USE_FOR_VIBRATE_RINGING_TYPE = (android.media.audio.common.AudioHalCapCriterion.FORCE_USE_FOR_VIBRATE_RINGING + SUFFIX) /* "ForceUseForVibrateRingingType" */;
}
