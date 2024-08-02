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
 * List of forced configurations aka device categories to be used in addition to the force use
 * in order to force the audio routing.
 *
 * {@hide}
 */
@Backing(type="int")
@SuppressWarnings(value={"redundant-name"})
@VintfStability
enum AudioPolicyForcedConfig {
    NONE = 0,
    SPEAKER = 1,
    HEADPHONES = 2,
    BT_SCO = 3,
    BT_A2DP = 4,
    WIRED_ACCESSORY = 5,
    BT_CAR_DOCK = 6,
    BT_DESK_DOCK = 7,
    ANALOG_DOCK = 8,
    DIGITAL_DOCK = 9,
    /** A2DP sink is not preferred to speaker or wired HS */
    NO_BT_A2DP = 10,
    /**
     * Sink selected to render system enforced sound in certain countries for legal reason.
     * (like camera shutter tone in Japan).
     */
    SYSTEM_ENFORCED = 11,
    HDMI_SYSTEM_AUDIO_ENFORCED = 12,
    ENCODED_SURROUND_NEVER = 13,
    ENCODED_SURROUND_ALWAYS = 14,
    ENCODED_SURROUND_MANUAL = 15,
    BT_BLE = 16,
}
