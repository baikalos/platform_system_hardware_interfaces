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

import android.media.audio.common.AudioHalCapCriterion;
import android.media.audio.common.AudioHalCapCriterionType;

/**
 * AudioHalCapRule defines the Configurable Audio Policy (CAP) rules for a given configuration.
 *
 * Rule may be compounded using a logical operator "ALL" (aka "AND") and "ANY' (aka "OR").
 * Compounded rule can be nested.
 * Rules on criterion is made of:
 *      -type of criterion:
 *              -inclusive -> match rules are "Includes" or "Excludes"
 *              -exclusive -> match rules are "Is" or "IsNot" aka equal or different
 *      -Name of the criterion must match the provided name in AudioHalCapCriterion
 *      -Value of the criterion must match the provided list of literal values from
 *         AudioHalCapCriterionType
 * Example of rule:
 *      ALL
 *          ANY
 *              ALL
 *                  CriterionRule1
 *                  CriterionRule2
 *              CriterionRule3
 *          CriterionRule4
 * Translated into:
 *     CompoundRule::ALL CompoundRule::ANY CompoundRule::ALL CriterionRule1 CriterionRule2
 *     Delimiter::END CriterionRule3 Delimiter::END CriterionRule4 Delimiter::END
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioHalCapRule {
    @VintfStability
    enum CompoundRule {
        INVALID = -1,
        /*
         * OR'ed rules
         */
        ANY = 0,
        /*
         * AND'ed rules
         */
        ALL,
    }

    @VintfStability
    enum MatchingRule {
        INVALID = -1,
        /*
         * Matching rule on exclusive criterion type.
         */
        IS = 0,
        /*
         * Exclusion rule on exclusive criterion type.
         */
        ISNOT,
        /*
         * Matching rule on inclusive criterion type (aka bitfield type).
         */
        INCLUDES,
        /*
         * Exclusion rule on inclusive criterion type (aka bitfield type).
         */
        EXCLUDES,
    }

    @VintfStability
    parcelable CriterionRule {
        MatchingRule matchingRule = MatchingRule.INVALID;
        /*
         * Must be one of the name defined by {@see AudioHalCapCriterion}.
         */
        @utf8InCpp String criterionName;
        /*
         * Must be one of the value defined by {@see AudioHalCapCriterionType} associated to the
         * {@see AudioHalCapCriterion.criterionTypeName}.
         */
        @utf8InCpp String criterionTypeValue;
    }
    /*
     * Defines the AND or OR'ed logcal rule between provided criterion rules if any and provided
     * nested rule if any.
     * Even if no rules or nestedRule are provided, a compound is expected with ALL rule to set the
     * rule of the {@see AudioHalConfiguration} as "always applicable".
     */
    CompoundRule compoundRule = CompoundRule.INVALID;
    /*
     * An AudioHalCapRule may contain 0..n CriterionRules.
     */
    @nullable CriterionRule[] criterionRules;
    /*
     * An AudioHalCapRule may be nested with 0..n AudioHalCapRules.
     */
    @nullable AudioHalCapRule[] nestedRules;
}