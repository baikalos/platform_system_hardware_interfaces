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

import android.media.audio.common.AudioHalCapDomain;

/**
 * AudioHalCapDomains defines the Configurable Audio Policy (CAP) engine configurable domains
 * that are used by the parameter-framework to define the values of the policy parameter according
 * to the values of criteria.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioHalCapDomains {
    /**
     * A non-empty list of configurable domains is mandatory for the configurable
     * audio policy (CAP) engine to define a dynamic management of policy engine parameters.
     */
    AudioHalCapDomain[] domains;
}
