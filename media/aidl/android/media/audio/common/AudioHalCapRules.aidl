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
 * AudioHalCapRules defines the Configurable Audio Policy (CAP) rules for a given configuration.
 *
 * Rules are compounded using a logical operator "ALL" (aka "AND") and "ANY' (aka "OR").
 * Compound rule can be nested.
 * Rules on criterion is made of:
 *      -type of criterion:
 *              -inclusive -> match rules are "Includes" or "Excludes"
 *              -exclusive -> match rules are "Is" or "IsNot" aka equal or different
 *      -Name of the criterion must match the provided name in AudioHalCapCriterion
 *      -Value of the criterion must match the provided list of literal values from
 *         AudioHalCapCriterionType
 * {@hide}
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioHalCapRules {
    @VintfStability
    enum CompoundRule {
        ANY = 0, /* OR'ed rules*/
        ALL,     /* AND'ed rules */
        INVALID,
    }

    @VintfStability
    enum Delimiter {
        END = 0,    /* End of compound rule section. */
    }

    @VintfStability
    enum MatchingRule {
        IS = 0,       /* Matching rule on exclusive criterion type. */
        ISNOT,        /* Exclysion rule on exclusive criterion type. */
        INCLUDES,     /* Matching rule on inclusive criterion type (aka bitfield type). */
        EXCLUDES,     /* Exclysion rule on inclusive criterion type (aka bitfield type). */
        INVALID,
    }

    @VintfStability
    parcelable CriterionRule {
        MatchingRule matchingRule = MatchingRule.INVALID;
        @utf8InCpp String criterionName;
        @utf8InCpp String criterionTypeValue;
    }

    @VintfStability
    parcelable NestedRule {
        @VintfStability
        union Operand {
            CompoundRule compoundRule = CompoundRule.INVALID;
            CriterionRule criterionRule;
            Delimiter delimiter = Delimiter.END;
        }
        Operand operand;
    }
    NestedRule[] nestedRules;
}