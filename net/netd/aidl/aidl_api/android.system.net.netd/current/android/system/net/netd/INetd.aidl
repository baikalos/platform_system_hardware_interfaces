///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.system.net.netd;
@VintfStability
interface INetd {
  android.system.net.netd.INetd.StatusCode addInterfaceToOemNetwork(in long networkHandle, in String ifname);
  android.system.net.netd.INetd.StatusCode addRouteToOemNetwork(in long networkHandle, in String ifname, in String destination, in String nexthop);
  android.system.net.netd.INetd.StatusCode createOemNetwork(out android.system.net.netd.INetd.OemNetwork network);
  android.system.net.netd.INetd.StatusCode destroyOemNetwork(in long networkHandle);
  android.system.net.netd.INetd.StatusCode removeInterfaceFromOemNetwork(in long networkHandle, in String ifname);
  android.system.net.netd.INetd.StatusCode removeRouteFromOemNetwork(in long networkHandle, in String ifname, in String destination, in String nexthop);
  android.system.net.netd.INetd.StatusCode setForwardingBetweenInterfaces(in String inputIfName, in String outputIfName, in boolean enable);
  android.system.net.netd.INetd.StatusCode setIpForwardEnable(in boolean enable);
  @Backing(type="int") @VintfStability
  enum StatusCode {
    OK = 0,
    INVALID_ARGUMENTS = 1,
    NO_NETWORK = 2,
    ALREADY_EXISTS = 3,
    PERMISSION_DENIED = 4,
    UNKNOWN_ERROR = 5,
  }
  parcelable OemNetwork {
    long networkHandle;
    int packetMark;
  }
}
