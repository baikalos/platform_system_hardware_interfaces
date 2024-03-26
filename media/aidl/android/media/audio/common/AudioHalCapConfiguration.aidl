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
 * AudioHalCapConfiguration defines the Configurable Audio Policy (CAP) groups of runtime
 * configurations that may affect a group of parameter within a domain.
 * The configuration is referred by its name and associated rule based on provided criteria.
 *
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioHalCapConfiguration {
    /**
     * Name of the configuration.
     */
    @utf8InCpp String name;
    /**
     * Rule to be verified to apply this configuration.
     * It is represented by a string that shall be interpreted by the parameter framework.
     *
     * Syntax must be respected:
     *   Allowed rule are logical operator "ALL" (aka "AND") and "ANY' (aka "OR").
     *   Any rule starts and ends with respectively "{" and "}" and is followed by
     *      -either a CSV list of matching rule on criterion/critera
     *      -or a CSV list of new logical rule(s).
     *
     *   The Matching rule on criterion must be consistent with:
     *      -type of criterion:
     *              -inclusive -> match rules are "Includes" or "Excludes"
     *              -exclusive -> match rules are "Is" or "IsNot" aka equal or different
     *      -Name of the criterion must match the provided name in AudioHalCapCriterion
     *      -Value of the criterion must match the provided list of literal values from
     *         AudioHalCapCriterionType
     */
    @utf8InCpp String rule;
}
