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
 * Contributed by: Giesecke & Devrient GmbH.
 */

package android.system.omapi;

import android.system.omapi.ISecureElementSession;

/**
 * SecureElement reader interface.
 *
 * @hide
 */
@VintfStability
interface ISecureElementReader {

    /**
     * Check if secure element is present or not.
     *
     * @return true if a card is present in the specified reader, otherwise false.
     */
    boolean isSecureElementPresent();

    /**
     * Connects to a secure element in this reader.
     *
     * This method prepares (initializes) the Secure Element for communication
     * before the Session object is returned (e.g. powers the Secure Element by
     * ICC ON if it is not already on). There might be multiple sessions opened at
     * the same time on the same reader. The system ensures the interleaving of
     * APDUs between the respective sessions.
     *
     * ## Error conditions
     * `SecureElementErrorCode::IO_ERROR` if there was an error communicating with
     *                            the secure element.
     *
     * @return A session object to be used to create Channels.
     */
    ISecureElementSession openSession();

    /**
     * Close all the sessions opened on this reader. All the channels opened by
     * all these sessions will be closed.
     */
    void closeSessions();

    /**
     * Closes all the sessions opened on this reader and resets the reader.
     * All the channels opened by all these sessions will be closed.
     *
     * @return true if the reset is successful, false otherwise.
     */
    boolean reset();
}
