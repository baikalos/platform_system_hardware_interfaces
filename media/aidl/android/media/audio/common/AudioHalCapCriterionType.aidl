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

import android.media.audio.common.AudioDeviceDescription;
import android.media.audio.common.AudioDeviceType;

/**
 * AudioHalCapCriterionType contains a criterion's possible numerical values
 * and their corresponding string literal representations. This is to be used
 * exclusively for the Configurable Audio Policy (CAP) engine configuration.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@SuppressWarnings(value={"redundant-name"})
@VintfStability
parcelable AudioHalCapCriterionType {
    /*
     * Inclusive (aka Bitfield) criteria allowing to identify available (connected) input / output
     * devices types.
     */
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES_TYPE = "InputDevicesMaskType";
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_TYPE = "OutputDevicesMaskType";
    /*
     * Inclusive (aka Bitfield) criteria allowing to identify available (connected) input /output
     * devices referred by their addresses.
     */
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES_ADDRESSES_TYPE =
            "OutputDevicesAddressesType";
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_ADDRESSES_TYPE =
            "InputDevicesAddressesType";
    /*
     * Exclusive criterion allowing to identify current mode.
     */
    const @utf8InCpp String TELEPHONY_MODE_TYPE = "AndroidModeType";
    /*
     * Exclusive criteria allowing to identify current forced usages.
     */
    const @utf8InCpp String FORCE_USE_FOR_COMMUNICATION_TYPE = "ForceUseForCommunicationType";
    const @utf8InCpp String FORCE_USE_FOR_MEDIA_TYPE = "ForceUseForMediaType";
    const @utf8InCpp String FORCE_USE_FOR_RECORD_TYPE = "ForceUseForRecordType";
    const @utf8InCpp String FORCE_USE_FOR_DOCK_TYPE = "ForceUseForDockType";
    const @utf8InCpp String FORCE_USE_FOR_SYSTEM_TYPE = "ForceUseForSystemType";
    const @utf8InCpp String FORCE_USE_FOR_HDMI_SYSTEM_AUDIO_TYPE
            = "ForceUseForHdmiSystemAudioType";
    const @utf8InCpp String FORCE_USE_FOR_ENCODED_SURROUND_TYPE
            = "ForceUseForEncodedSurroundType";
    const @utf8InCpp String FORCE_USE_FOR_VIBRATE_RINGING_TYPE
            = "ForceUseForVibrateRingingType";

    /**
     * Name is used to associate an AudioHalCapCriterionType with an
     * AudioHalCapCriterion.
     */
    @utf8InCpp String name;
    /**
     * If true, indicates that the criterion type supports more than one value
     * at a time (e.g. a criterion type that represents a mask/bitfield).
     * Otherwise, the criterion type can only have one value at a time.
     */
    boolean isInclusive;
    /**
     * List of all possible criterion values represented as human-readable
     * string literals. These strings must only contain ascii characters,
     * and the client must never attempt to parse them.
     * The string value is just a tag that will be found in the {@see AudioHalCapRule} expression,
     * and associated to a numerical value that will be provided/set by the configurable engine.
     * The expected values will depends on the criterion type:
     *   -Input/Output device type: human readable type name
     *   -Input/Output device address: device address as provided by
     *          {@see android.hardware.audio.core.IModule#getAudioPortConfigs}
     *   -Telephony mode: human readable mode name
     *   -forced usage: human readable forced usage name.
     */
    @utf8InCpp String[] values;
    /**
     * List of all possible criterion values represented as numerical value intented to be used
     * by the parameter-framework for performance issue.
     * Each human readable value shall have its associated numerical value.
     */
    @nullable long[] numericalValue;
    /**
     * Optional list of associated numerical value in Android to the numerical value used by the
     * parameter framework. It is only applicable for Device representation as a bitfield in
     * parameter framework. Only one bit is expected to represent a device. A lookup table is
     * hence required to translate android type to parameter framework device representation.
     */
    @nullable AudioDeviceDescription[] androidMappedValue;
}
