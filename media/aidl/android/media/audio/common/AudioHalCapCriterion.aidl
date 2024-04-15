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

package android.media.audio.common;

/**
 * AudioHalCapCriterion is a wrapper for a CriterionType and its default value.
 * This is to be used exclusively for the Configurable Audio Policy (CAP) engine
 * configuration.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@SuppressWarnings(value={"redundant-name"})
@VintfStability
parcelable AudioHalCapCriterion {
    /*
     * Inclusive (aka Bitfield) criteria allowing to identify available (connected) input / output
     * devices types.
     */
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES = "AvailableInputDevices";
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES = "AvailableOutputDevices";
    /*
     * Inclusive (aka Bitfield) criteria allowing to identify available (connected) input /output
     * devices referred by their addresses.
     */
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES_ADDRESSES =
            "AvailableInputDevicesAddresses";
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_ADDRESSES =
            "AvailableOutputDevicesAddresses";
    /*
     * Exclusive criterion allowing to identify current mode.
     */
    const @utf8InCpp String TELEPHONY_MODE = "TelephonyMode";
    /*
     * Exclusive criteria allowing to identify current forced usages.
     */
    const @utf8InCpp String FORCE_USE_FOR_COMMUNICATION = "ForceUseForCommunication";
    const @utf8InCpp String FORCE_USE_FOR_MEDIA = "ForceUseForMedia";
    const @utf8InCpp String FORCE_USE_FOR_RECORD = "ForceUseForRecord";
    const @utf8InCpp String FORCE_USE_FOR_DOCK = "ForceUseForDock";
    const @utf8InCpp String FORCE_USE_FOR_SYSTEM = "ForceUseForSystem";
    const @utf8InCpp String FORCE_USE_FOR_HDMI_SYSTEM_AUDIO = "ForceUseForHdmiSystemAudio";
    const @utf8InCpp String FORCE_USE_FOR_ENCODED_SURROUND = "ForceUseForEncodedSurround";
    const @utf8InCpp String FORCE_USE_FOR_VIBRATE_RINGING = "ForceUseForVibrateRinging";

    @utf8InCpp String name;
    /**
     * Used to map the AudioHalCapCriterion to an AudioHalCapCriterionType with
     * the same name.
     */
    @utf8InCpp String criterionTypeName;
    /**
     * This is the default value of a criterion represented as a string literal.
     * An empty string must be used for inclusive criteria (i.e. criteria that
     * are used to represent a bitfield). The client must never attempt to parse
     * the value of this field.
     */
    @utf8InCpp String defaultLiteralValue;
}
