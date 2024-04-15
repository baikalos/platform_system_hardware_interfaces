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

import android.media.audio.common.AudioDeviceAddress;
import android.media.audio.common.AudioDeviceDescription;
import android.media.audio.common.AudioMode;
import android.media.audio.common.AudioPolicyForcedConfig;
import android.media.audio.common.AudioPolicyForceUse;

/**
 * AudioHalCapCriterion is a wrapper for a CriterionType and its default value.
 * This is to be used exclusively for the Configurable Audio Policy (CAP) engine
 * configuration.
 *
 * {@hide}
 */
@VintfStability
union AudioHalCapCriterionV2 {
    /**
     * A criterion can either be exclusive (can take one value at a time) or inclusive (like a
     * bitfield, it can have several values). Rules expected on inclusive or exclusive will be
     * different.
     */
    @VintfStability
    enum LogicalDisjunction {
        EXCLUSIVE = 0,
        INCLUSIVE,
    }
    /**
     * Forced configuration for a given usage criterion. It is used to force the audio routing
     * for a given use case or usage.
     */
    @VintfStability
    parcelable ForceConfigForUse {
        /**  Force usage addressed by this criterion. */
        AudioPolicyForceUse forceUse = AudioPolicyForceUse.MEDIA;
        /** List of supported value by this criterion. */
        AudioPolicyForcedConfig[] values;
        /** Default configuration applied if none is provided. */
        AudioPolicyForcedConfig defaultValue = AudioPolicyForcedConfig.NONE;
        /** Logic followed by this criterion, only one value at given time. */
        LogicalDisjunction logic = LogicalDisjunction.EXCLUSIVE;
    }
    /**
     * The telephony mode or call state criterion. It is used to apply specific audio routing for
     * the telephony use cases.
     */
    @VintfStability
    parcelable TelephonyMode {
        /** List of supported audio mode values for this criterion. */
        AudioMode[] values;
        /** Default value to be applied if none is provided. */
        AudioMode defaultValue = AudioMode.NORMAL;
        /** Logic followed by this criterion, only one value at given time. */
        LogicalDisjunction logic = LogicalDisjunction.EXCLUSIVE;
    }
    @VintfStability
    parcelable AvailableDevices {
        /** List if supported values (aka audio devices) by this criterion. */
        AudioDeviceDescription[] values;
        /** Logic followed by this criterion, multiple devices can be selected/available. */
        LogicalDisjunction logic = LogicalDisjunction.INCLUSIVE;
    }
    @VintfStability
    parcelable AvailableDevicesAddresses {
        /** List if supported values (aka audio device addresses) by this criterion. */
        AudioDeviceAddress[] values;
         /** Logic followed by this criterion, multiple device addresses can be available. */
        LogicalDisjunction logic = LogicalDisjunction.INCLUSIVE;
    }
    AvailableDevices availableInputDevices;
    AvailableDevices availableOutputDevices;
    AvailableDevicesAddresses availableInputDevicesAddresses;
    AvailableDevicesAddresses availableOutputDevicesAddresses;
    TelephonyMode telephonyMode;
    ForceConfigForUse forceUseForCommunication;
    ForceConfigForUse forceUseForMedia;
    ForceConfigForUse forceUseForRecord;
    ForceConfigForUse forceUseForDock;
    ForceConfigForUse forceUseForSystem;
    ForceConfigForUse forceUseForHdmiSystemAudio;
    ForceConfigForUse forceUseForEncodedSurround;
    ForceConfigForUse forceUseForVibrateRinging;

    /**
     * Supported criterion types for Configurable Audio Policy Engine.
     */
    @VintfStability
    union Type {
        AudioDeviceDescription availableDevicesType;
        AudioDeviceAddress availableDevicesAddressesType;
        AudioMode telephonyModeType;
        AudioPolicyForcedConfig forcedConfigType;
    }
    Type type;
}