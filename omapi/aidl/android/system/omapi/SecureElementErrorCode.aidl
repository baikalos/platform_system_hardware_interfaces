/*
 * Copyright (C) 2021, The Android Open Source Project
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
/*
 * Copyright (c) 2015-2017, The Linux Foundation.
 */
/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package android.system.omapi;

/**
 * @hide
 */
@VintfStability
@Backing(type = "int")
enum SecureElementErrorCode {
    /**
     * Indicates there was an error communicating with the Secure Element.
     */
    IO_ERROR = 1,
    /**
     * If AID cannot be selected or is not available when opening a logical
     * channel then this error code is returned.
     */
    NO_SUCH_ELEMENT_ERROR = 2,
    /**
     * Unknown Error.
     */
    UNKNOWN_ERROR = 100
}
