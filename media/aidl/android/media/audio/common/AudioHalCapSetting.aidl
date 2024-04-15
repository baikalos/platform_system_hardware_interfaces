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

import android.media.audio.common.AudioHalCapParameter;

/**
 * AudioHalCapSetting defines the Configurable Audio Policy (CAP) groups of runtime
 * settings aka the value of parameter within a domain for a given configuration.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioHalCapSetting {
    @VintfStability
    parcelable ParameterSetting {
        @utf8InCpp String path;
        AudioHalCapParameter name = AudioHalCapParameter.SYS_RESERVED_INVALID;
        /**
         * Parameter setting value must be one of the supported type.
         */
        @VintfStability
        union Value {
            boolean booleanType;
            int integerType;
            double doubleType;
            float floatingPointType;
            @utf8InCpp String stringType;
        }
        Value value;
    }
    /**
     * Name of the configurable domain. Must be unique for given instance of parameter framework.
     */
    @utf8InCpp String configurationName;
    /**
     * A non-empty list of parameter settings, aka a couple of parameter and values.
     */
    ParameterSetting[] parameterSettings;
}
