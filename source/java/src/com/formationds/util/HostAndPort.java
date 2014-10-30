package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.net.URI;
import java.net.URISyntaxException;

public class HostAndPort {
    private int port;
    private String host;

    public HostAndPort(String host, int port) {
        setHost(host);
        setPort(port);
    }

    public static HostAndPort parseWithDefaultPort(String hostAndPort, int defaultPortIfAbsent) {
        URI uri = null;
        try {
            uri = new URI("http://" + hostAndPort);
        } catch (URISyntaxException e) {
            throw new IllegalArgumentException("Invalid host and port syntax (should be host:port)");
        }
        return new HostAndPort(uri.getHost(), uri.getPort() == -1 ? defaultPortIfAbsent : uri.getPort());
    }

    @Override
    public String toString() {
        return host + ":" + port;
    }

    @Override
    public int hashCode() {
        return Integer.hashCode(getPort()) ^ getHost().hashCode();
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof HostAndPort))
            return false;

        HostAndPort other = (HostAndPort) obj;
        return getHost().equals(other.getHost()) && getPort() == other.getPort();
    }

    public String getHost() {
        return host;
    }

    public void setHost(String host) {
        this.host = host;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }
}
