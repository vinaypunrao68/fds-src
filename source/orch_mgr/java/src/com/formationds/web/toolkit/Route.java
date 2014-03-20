package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.util.function.Supplier;

public class Route {
    private Request request;
    private Supplier<RequestHandler> handler;

    public Route(Request request, Supplier<RequestHandler> handler) {
        this.request = request;
        this.handler = handler;
    }

    public Request getRequest() {
        return request;
    }

    public Supplier<RequestHandler> getHandler() {
        return handler;
    }
}
