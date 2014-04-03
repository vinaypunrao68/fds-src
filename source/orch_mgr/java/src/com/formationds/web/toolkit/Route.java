package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.util.Map;
import java.util.function.Supplier;

public class Route {
    private Request request;
    private Map<String, String> attributes;
    private Supplier<RequestHandler> handler;

    public Route(Request request, Map<String, String> attributes, Supplier<RequestHandler> handler) {
        this.request = request;
        this.attributes = attributes;
        this.handler = handler;
    }

    public Request getRequest() {
        return request;
    }

    public Supplier<RequestHandler> getHandler() {
        return handler;
    }

    public Map<String, String> getAttributes() {
        return attributes;
    }
}
