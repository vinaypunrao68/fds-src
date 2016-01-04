/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Objects;


public class Node extends AbstractResource<Long> {

    // TODO: what are the supported node states.
    public enum NodeState { UNKNOWN, UP, DOWN, REMOVED, DISCOVERED, MIGRATION, STANDBY  }

    /**
     *
     */
    public static class NodeStatus {
        private final Instant timestamp;
        private final NodeState currentState;

        public NodeStatus( NodeState currentState ) {
            this(Instant.now(), currentState);
        }

        public NodeStatus( Instant timestamp, NodeState currentState ) {
            this.timestamp = timestamp;
            this.currentState = currentState;
        }

        public NodeState getCurrentState() {
            return currentState;
        }

        public Instant getTimestamp() {
            return timestamp;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof NodeStatus) ) { return false; }
            final NodeStatus nodeStatus = (NodeStatus) o;
            return Objects.equals( timestamp, nodeStatus.timestamp ) &&
                   Objects.equals( currentState, nodeStatus.currentState );
        }

        @Override
        public int hashCode() {
            return Objects.hash( timestamp, currentState );
        }
    }

    public static class NodeCapacity {
        private final Size ssd;
        private final Size hdd;
        private final Size total;

        public NodeCapacity( final Size ssd, final Size hdd, final Size total )
        {
            this.ssd = ssd;
            this.hdd = hdd;
            this.total = total;
        }

        public Size getSSD( )
        {
            return ssd;
        }

        public Size getHDD( )
        {
            return hdd;
        }

        public Size getTotal( )
        {
            return total;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof NodeCapacity) ) { return false; }
            final NodeCapacity nodeCapacity = (NodeCapacity) o;
            return Objects.equals( ssd, nodeCapacity.ssd ) &&
                   Objects.equals( hdd, nodeCapacity.hdd ) &&
                   Objects.equals( total, nodeCapacity.total );
        }

        @Override
        public int hashCode() {
            return Objects.hash( ssd, hdd, total );
        }
    }

    /**
     * A node may be addressed by one or both of an IPv4 or IPv6 address.
     */
    public static class NodeAddress {

        /**
         *
         * @param addr the host name or address (IPv4 or IPv6)
         * @return the InetAddress
         * @throws UnknownHostException if it fails to create an InetAddress based on the address string
         */
        public static InetAddress fromString(String addr) throws UnknownHostException {
            return InetAddress.getByName( addr );
        }

        /**
         * @param ipv4 an IPv4 Address encoded as an integer
         * @return the Inet4Address with the IPv4 address
         */
        public static Inet4Address ipv4Address( int ipv4 ) {
            try {
                byte[] b = ByteBuffer.allocate( 4 ).putInt( ipv4 ).array();
                return (Inet4Address)InetAddress.getByAddress( b );
            } catch (UnknownHostException uhe) {
                // not really possible since the only validation documented by getByAddress is that
                // it checks that the length is correct.
                throw new IllegalArgumentException( "Failed to convert to an IPv4 address", uhe );
            }
        }

        /**
         *
         * @param ipv6hi high bits of the IPv6 address
         * @param ipv6low low bits of the IPv6 address
         * @return the Inet6Address with the IPv6 address
         */
        public static Inet6Address ipv6Address( long ipv6hi, long ipv6low ) {
            try {
                byte[] b = ByteBuffer.allocate( 16 ).putLong( ipv6hi ).putLong( ipv6low ).array();
                return (Inet6Address)InetAddress.getByAddress( b );
            } catch (UnknownHostException uhe) {
                // not really possible since the only validation documented by getByAddress is that
                // it checks that the length is correct.
                throw new IllegalArgumentException( "Failed to convert to an IPv4 address", uhe );
            }
        }

        /**
         * @param ipv4 an IPv4 Address encoded as an integer
         * @return the NodeAddress with the IPv4 address
         */
        public static NodeAddress ipv4NodeAddress(int ipv4) {
            return new NodeAddress( ipv4Address( ipv4 ) );
        }

        /**
         *
         * @param ipv6hi high bits of the IPv6 address
         * @param ipv6lo low bits of the IPv6 address
         * @return the NodeAddress with the IPv6 address
         */
        public static NodeAddress ipv6NodeAddress(long ipv6hi, long ipv6lo) {
            return new NodeAddress( ipv6Address( ipv6hi, ipv6lo ) );
        }

        /**
         *
         * @param ipv4 an IPv4 Address encoded as an integer
         * @param ipv6hi high bits of the IPv6 address
         * @param ipv6lo low bits of the IPv6 address
         * @return the NodeAddress with the IPv4 and IPv6 address
         */
        public static NodeAddress dualAddress(int ipv4, long ipv6hi, long ipv6lo) {
            Inet4Address v4 = ipv4Address( ipv4 );
            Inet6Address v6 = ipv6Address( ipv6hi, ipv6lo );
            return new NodeAddress( v4, v6 );
        }

        final private Inet4Address ipv4Address;
        final private Inet6Address ipv6Address;

        /**
         * @param ipv4 the ipv4 address
         */
        public NodeAddress(Inet4Address ipv4) {
            this(ipv4, null);
        }

        /**
        * @param ipv6 the ipv6 address
         */
        public NodeAddress(Inet6Address ipv6) {
            this(null, ipv6);
        }

        /**
         * @param ipv4 the node's ipv4 address
         * @param ipv6 the node's ipv6 address
         */
        public NodeAddress(Inet4Address ipv4, Inet6Address ipv6) {
            this.ipv4Address = ipv4;
            this.ipv6Address = ipv6;
        }

        /**
         * @return the IPv4 address or null if not set
         */
        public Inet4Address getIpv4Address(){
        	return ipv4Address;
        }

        /**
         *
         * @return the IPv6 address or null if not set
         */
        public Inet6Address getIpv6Address(){
        	return ipv6Address;
        }

        /**
         * Get the host IP address as a string, preferring an IPv4 address if set.
         *
         * @return the host ip address as a string
         */
        public String getHostAddress() {
            if (ipv4Address != null) {
                return ipv4Address.getHostAddress();
            }
            else if (ipv6Address != null) {
                return ipv6Address.getHostAddress();
            }
            else {
                throw new IllegalStateException( "No address defined on node."  );
            }
        }
        @Override
        public String toString() {
            final StringBuilder sb = new StringBuilder();
            if (ipv4Address != null) sb.append( "ipv4=" ).append( ipv4Address.getHostAddress() );
            if (ipv4Address != null && ipv6Address != null) sb.append( "; " );
            if (ipv6Address != null) sb.append( "ipv6=" ).append( ipv6Address.getHostAddress() );
            return sb.toString();
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof NodeAddress) ) { return false; }
            final NodeAddress that = (NodeAddress) o;
            return Objects.equals( ipv4Address, that.ipv4Address ) &&
                   Objects.equals( ipv6Address, that.ipv6Address );
        }

        @Override
        public int hashCode() {
            return Objects.hash( ipv4Address, ipv6Address );
        }
    }

    // TODO: not in the model, but do we need internal address vs. external for config?
    private NodeAddress address;
    private NodeState state;
    private Map<ServiceType, List<Service>> serviceMap;
    private Size ssdCapacity;
    private Size diskCapacity;

    protected Node() {}

    /**
     *
     * @param uid the node unique id
     * @param address the node address
     * @param state the current node state
     * @param diskCapacity the nodes disk capacity
     * @param ssdCapacity the node SSD capacity
     * @param services the node's current service map
     */
    public Node( Long uid, NodeAddress address, NodeState state,
                 Size diskCapacity, Size ssdCapacity,
                 Map<ServiceType, List<Service>> services ) {
        super( uid, address.getHostAddress() );
        this.address = address;
        this.state = state;
        this.diskCapacity = diskCapacity;
        this.ssdCapacity = ssdCapacity;
        this.serviceMap = services;
    }

    /**
     * @param address the node address
     */
    public void setAddress( NodeAddress address ) {
        this.address = address;
    }

    /**
     * @param state the node state
     */
    public void setState( NodeState state ) {
        this.state = state;
    }

    /**
     * @param serviceMap the current service map
     */
    public void setServiceMap( Map<ServiceType, List<Service>> serviceMap ) {
        this.serviceMap = serviceMap;
    }

    /**
     * @return the node address
     */
    public NodeAddress getAddress() {
        return address;
    }

    /**
     *
     * @return the node state
     */
    public NodeState getState() {
        return state;
    }

    /**
     * @return the node service map
     */
    public Map<ServiceType, List<Service>> getServices() {
        return serviceMap;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof Node) ) { return false; }
        if ( !super.equals( o ) ) { return false; }
        final Node node = (Node) o;
        return Objects.equals( address, node.address ) &&
               Objects.equals( state, node.state ) &&
               Objects.equals( serviceMap, node.serviceMap );
    }

    @Override
    public int hashCode() {
        return Objects.hash( super.hashCode(), address, state, serviceMap );
    }

    /**
     *
     * @return the SSD capacity
     */
    public Size getSsdCapacity() {
        return ssdCapacity;
    }

    /**
     *
     * @return the Disk capacity
     */
    public Size getDiskCapacity() {
        return diskCapacity;
    }
}
