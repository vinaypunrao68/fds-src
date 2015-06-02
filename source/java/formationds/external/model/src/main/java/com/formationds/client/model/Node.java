/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.util.List;
import java.util.Map;
import java.util.Objects;

public class Node extends AbstractResource<Long> {

    public static class NodeState {

    }

    public static class NodeAddress {
        final private Inet4Address ipv4Address;
        final private Inet6Address ipv6Address;

        public NodeAddress(Inet4Address ipv4) {
            this(ipv4, null);
        }
        public NodeAddress(Inet6Address ipv6) {
            this(null, ipv6);
        }
        public NodeAddress(Inet4Address ipv4, Inet6Address ipv6) {
            this.ipv4Address = ipv4;
            this.ipv6Address = ipv6;
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

    public Node(Long id, String name) {
        super(id, name);
    }
}
