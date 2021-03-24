/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.system.memtrack_proxy;

import android.hardware.memtrack.DeviceInfo;
import android.hardware.memtrack.MemtrackRecord;
import android.hardware.memtrack.MemtrackType;

/**
 * Memtrack Proxy arbitrates requests to Memtrack HAL APIs.
 *
 * Its main purpose is to enforce access control on the information
 * provided by the Memtrack HAL, as described in the interface below.
 */

interface IMemtrackProxy {
    /**
     * getMemory() Returns the results of IMemtrack::getMemory()
     * if the calling UID is AID_SYSTEM or AID_ROOT or the requested
     * PID matches the calling PID. Else returns a status with
     * exception code EX_SECURITY.
     *
     * @param pid process for which memory information is requested
     * @param type memory type that information is being requested about
     * @return vector of MemtrackRecord containing memory information
     */
    MemtrackRecord[] getMemory(in int pid, in MemtrackType type);

    /**
     * getGpuDeviceInfo() Returns the results of
     * IMemtrack::getGpuDeviceInfo() if the calling UID is AID_SYSTEM
     * or AID_ROOT. Else returns a status with exception code EX_SECURITY.
     *
     * @return vector of DeviceInfo populated for all GPU devices.
     */
    DeviceInfo[] getGpuDeviceInfo();
}
