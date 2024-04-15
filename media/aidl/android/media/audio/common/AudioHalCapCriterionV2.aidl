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
    @VintfStability
    enum LogicalDisjunction {
        EXLUSIVE = 0,
        INCLUSIVE,
    }
    @VintfStability
    parcelable ForceConfigForUse {
        AudioPolicyForceUse forceUse = AudioPolicyForceUse.MEDIA;
        @nullable AudioPolicyForcedConfig[] values;
        AudioPolicyForcedConfig defaultValue = AudioPolicyForcedConfig.NONE;
        LogicalDisjunction logic = LogicalDisjunction.EXLUSIVE;
    }
    @VintfStability
    parcelable TelephonyMode {
        @nullable AudioMode[] values;
        AudioMode defaultValue = AudioMode.NORMAL;
        LogicalDisjunction logic = LogicalDisjunction.EXLUSIVE;
    }
    @VintfStability
    parcelable AvailableDevices {
        @nullable AudioDeviceDescription[] values;
        LogicalDisjunction logic = LogicalDisjunction.INCLUSIVE;
    }
    @VintfStability
    parcelable AvailableDevicesAddresses {
        @nullable AudioDeviceAddress[] values;
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

    @VintfStability
    union Type {
        AudioDeviceDescription availableDevicesType;
        AudioDeviceAddress availableDevicesAddressesType;
        AudioMode telephonyModeType;
        AudioPolicyForcedConfig forcedConfigType;
    }
    Type type;
}