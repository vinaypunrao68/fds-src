package com.formationds.samplewebapp;

import baggage.hypertoolkit.RequestHandler;
import baggage.hypertoolkit.views.Resource;
import baggage.hypertoolkit.views.TextResource;

import javax.servlet.http.HttpServletRequest;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Hello implements RequestHandler {
    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        return new TextResource("Hello");
    }

    @Override
    public boolean log() {
        return false;
    }
}
