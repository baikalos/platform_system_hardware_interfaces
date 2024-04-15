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
import android.media.audio.common.AudioHalCapCriterion;

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
    const @utf8InCpp String SUFFIX = "Type";
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES_TYPE =
            AudioHalCapCriterion.AVAILABLE_INPUT_DEVICES + SUFFIX;
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_TYPE =
            AudioHalCapCriterion.AVAILABLE_OUTPUT_DEVICES + SUFFIX;
    const @utf8InCpp String AVAILABLE_INPUT_DEVICES_ADDRESSES_TYPE =
            AudioHalCapCriterion.AVAILABLE_INPUT_DEVICES_ADDRESSES + SUFFIX;
    const @utf8InCpp String AVAILABLE_OUTPUT_DEVICES_ADDRESSES_TYPE =
            AudioHalCapCriterion.AVAILABLE_OUTPUT_DEVICES_ADDRESSES + SUFFIX;
    const @utf8InCpp String TELEPHONY_MODE_TYPE = AudioHalCapCriterion.TELEPHONY_MODE + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_COMMUNICATION_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_COMMUNICATION + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_MEDIA_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_MEDIA + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_RECORD_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_RECORD + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_DOCK_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_DOCK + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_SYSTEM_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_SYSTEM + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_HDMI_SYSTEM_AUDIO_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_HDMI_SYSTEM_AUDIO + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_ENCODED_SURROUND_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_ENCODED_SURROUND + SUFFIX;
    const @utf8InCpp String FORCE_USE_FOR_VIBRATE_RINGING_TYPE =
            AudioHalCapCriterion.FORCE_USE_FOR_VIBRATE_RINGING + SUFFIX;

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
     * parameter framework. It is only applicable for Device criterion type.
     */
    @nullable AudioDeviceDescription[] androidMappedValue;
}
