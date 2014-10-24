package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class HostAndPort {
    private int port;
    private String host;

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
        if(!(obj instanceof HostAndPort))
            return false;

        HostAndPort other = (HostAndPort) obj;
        return getHost().equals(other.getHost()) && getPort() == other.getPort();
    }

    public HostAndPort(String hostAndPort) {
        String[] parts = hostAndPort.split(":");
        if(parts.length != 2)
            throw new IllegalArgumentException("expecting hostAndPort to contain one and only one colon, instead got: '" + hostAndPort + "'");
        setHost(parts[0]);
        setPort(Integer.parseInt(parts[1]));
    }

    public HostAndPort(String host, int port) {
        setHost(host);
        setPort(port);
    }

    public static HostAndPort parseWithDefaultPort(String hostAndPort, int defaultPortIfAbsent) {
        String[] parts = hostAndPort.split(":");
        if(parts.length > 2)
            throw new IllegalArgumentException("too many colons in host specifier, got: '" + hostAndPort + "'");
        if(parts.length == 2)
            return new HostAndPort(parts[0], Integer.parseInt(parts[1]));

        return new HostAndPort(parts[0], defaultPortIfAbsent);
    }

    public String getHost() {
        return host;
    }

    public void setHost(String host) {
        if(host.contains(":"))
            throw new IllegalArgumentException("host may not contain a colon character");
        this.host = host;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }
}
