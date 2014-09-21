package com.formationds.web.toolkit;

/**
* Copyright (c) 2014 Formation Data Systems.
* All rights reserved.
*/
public class HttpConfiguration {
    private int port;
    private String host;

    public HttpConfiguration(int port, String host) {
        this.port = port;
        this.host = host;
    }

    public HttpConfiguration(int port) {
        this(port, "0.0.0.0");
    }

    public int getPort() {
        return port;
    }

    public String getHost() {
        return host;
    }
}
