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
 * SecureElement channel interface
 *
 * @hide
 */
@VintfStability
interface ISecureElementChannel {

    /**
     * Closes the specified connection and frees internal resources.
     * A logical channel will be closed.
     */
    void close();

    /**
     * Tells if this channel is closed.
     *
     * @return true if the channel is closed, false otherwise.
     */
    boolean isClosed();

    /**
     * Returns a boolean telling if this channel is the basic channel.
     *
     * @return true if this channel is a basic channel, false otherwise.
     */
    boolean isBasicChannel();

    /**
     * Returns the data as received from the application select command
     * inclusively the status word. The returned byte array contains the data
     * bytes in the following order:
     * [<first data byte>, ..., <last data byte>, <sw1>, <sw2>]
     *
     * @return Response as a byte array returned from application select command.
     */
    byte[] getSelectResponse();

    /**
     * Transmits the specified command APDU and returns the response APDU.
     * MANAGE channel commands are not supported. Selection of applets is
     * not supported in logical channels.
     *
     * ## Error conditions
     * `SecureElementErrorCode::IO_ERROR` if there was an error communicating with
     *                            the secure element.
     *
     * @return Response APDU in a byte array.
     */
    byte[] transmit(in byte[]command);

    /**
     * Performs a selection of the next Applet on this channel that matches to
     * the partial AID specified in the openBasicChannel(byte[] aid) or
     * openLogicalChannel(byte[] aid) method. This mechanism can be used by a
     * device application to iterate through all Applets matching to the same
     * partial AID.
     *
     * If selectNext() returns true a new Applet was successfully selected on
     * this channel. If no further Applet exists with matches to the partial AID
     * this method returns false and the already selected Applet stays selected.
     *
     * ## Error conditions
     * `SecureElementErrorCode::IO_ERROR` if there was an error communicating with
     *                            the secure element.
     *
     * @return true if new Applet was successfully selected, false if no further
     * Applet exists which matches the partial AID.
     */
    boolean selectNext();
}
