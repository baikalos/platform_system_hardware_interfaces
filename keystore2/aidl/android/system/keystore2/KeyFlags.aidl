/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system.keystore2;

/**
 * Additional switches, that can be applied to during key generation and import.
 */
@VintfStability
@Backing(type="int")
enum KeyFlags {
    /**
     * Convenience variant indicating the absence of any additional switches.
     */
    NONE = 0,
    /**
     * This flag disables cryptographic binding to the LSKF for auth bound keys.
     */
    AUTH_BOUND_WITHOUT_CRYPTOGRAPHIC_LSKF_BINDING = 0x1,
 }
