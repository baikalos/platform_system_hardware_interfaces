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
     * Indicates the aid is out of range. An AID is defined in ISO 7816-5
     * to be a sequence of bytes between 5 and 16 bytes in length.
     */
    AID_OUT_OF_RANGE = 3,
    /**
     * Indicates caller has not specified his package names.
     */
    PACKAGE_NAMES_NOT_SPECIFIED = 4,
    /**
     * Indicates that a channel is being opened on a closed session.
     */
    SESSION_CLOSED = 5,
    /**
     * Indicates that a listener object is not passed.
     */
    MISSING_LISTENER = 6,
    /**
     * If the protocol byte P2 (which is part of ISO 7816) is unsupported
     * this error is returned.
     */
    UNSUPPORTED_P2_BYTE = 7,
    /**
     * Unknown Error.
     */
    UNKNOWN_ERROR = 100
}
