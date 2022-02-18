// FIXME: license file, or use the -l option to generate the files with the header.

package android.system.net.netd;

/**
 * This is the root of the HAL module and is the interface returned when
 * loading an implementation of the INetd HAL.
 */
// Interface inherits from android.system.net.netd@1.0::INetd but AIDL does not support interface inheritance.
@VintfStability
interface INetd {
    /**
     * Return values for INetd requests
     */
    @VintfStability
    @Backing(type="int")
    enum StatusCode {
        OK,
        INVALID_ARGUMENTS,
        NO_NETWORK,
        ALREADY_EXISTS,
        PERMISSION_DENIED,
        UNKNOWN_ERROR,
    }
    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Add interface to a specified OEM network
     *
     * @param networkHandle Handle to the OEM network previously returned from
     *                      @1.0::createOemNetwork.
     * @param ifname        Interface name to add to the OEM network.
     *                      For e.g. "dummy0".
     */
    StatusCode addInterfaceToOemNetwork(in long networkHandle, in String ifname);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Add route to a specified OEM network
     * Either both, or one of the ifname and nexthop must be specified.
     *
     * @param networkHandle Handle to the OEM network previously returned from
     *                      @1.0::createOemNetwork.
     * @param ifname        Interface name specified by the route, or an empty
     *                      string for a route that does not specify an
     *                      interface.
     *                      For e.g. "dummy0"
     * @param destination   The destination prefix of the route in CIDR notation.
     *                      For e.g. 192.0.2.0/24 or 2001:db8:1::/48.
     * @param nexthop       IP address of the gateway for the route, or an empty
     *                      string for a directly-connected route. If non-empty,
     *                      must be of the same address family as the
     *                      destination.
     *                      For e.g. 10.0.0.1 or 2001:db8:1::cafe.
     */
    StatusCode addRouteToOemNetwork(in long networkHandle, in String ifname, in String destination,
        in String nexthop);

    // FIXME: AIDL does not allow long to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow int to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Creates a physical network to be used for interfaces managed by the OEM
     *
     * @param out networkHandle a handle to the OEM network. Zero implies
     *             no networks are available and created
     * @param out packetMark The packet mark that vendor network stack configuration code must use when
     *             routing packets to this network. No applications may use this mark. They must
     *             instead pass the networkHandle to the android_set*network LL-NDK APIs.
     * @param out status operation status
     */
    void createOemNetwork(out long networkHandle, out int packetMark, out StatusCode status);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Destroys the specified network previously created using createOemNetwork()
     *
     * @return operation status
     */
    StatusCode destroyOemNetwork(in long networkHandle);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Remove interface from a specified OEM network.
     *
     * @param networkHandle Handle to the OEM network previously returned from
     *                      @1.0::createOemNetwork.
     * @param ifname        Interface name to remove from the OEM network.
     *                      For e.g. "dummy0".
     */
    StatusCode removeInterfaceFromOemNetwork(in long networkHandle, in String ifname);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Remove route from a specified OEM network.
     * Either both, or one of the ifname and nexthop must be specified.
     *
     * @param networkHandle Handle to the OEM network previously returned from
     *                      @1.0::createOemNetwork.
     * @param ifname        Interface name specified by the route, or an empty
     *                      string for a route that does not specify an
     *                      interface.
     *                      For e.g. "dummy0"
     * @param destination   The destination prefix of the route in CIDR notation.
     *                      For e.g. 192.0.2.0/24 or 2001:db8:1::/48.
     * @param nexthop       IP address of the gateway for the route, or an empty
     *                      string for a directly-connected route. If non-empty,
     *                      must be of the same address family as the
     *                      destination.
     *                      For e.g. 10.0.0.1 or 2001:db8:1::cafe.
     */
    StatusCode removeRouteFromOemNetwork(in long networkHandle, in String ifname,
        in String destination, in String nexthop);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Enables forwarding between two interfaces, one of which must be in an
     * OEM network.
     *
     * @param inputIfName  Input interface. For e.g. "dummy0".
     * @param outputIfName Output interface. For e.g. "rmnet_data7".
     * @param enable       bool to enable or disable forwarding between the
     *                     two interfaces.
     */
    StatusCode setForwardingBetweenInterfaces(in String inputIfName, in String outputIfName,
        in boolean enable);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    // Adding return type to method instead of out param StatusCode status since there is only one return value.
    /**
     * Enable IP forwarding on the system. Client must disable forwarding when
     * it's no longer needed.
     *
     * @param enable    bool to enable or disable forwarding.
     */
    StatusCode setIpForwardEnable(in boolean enable);
}
