package com.formationds.web.toolkit;

import org.eclipse.jetty.server.Request;

import java.util.Map;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class FourOhFour extends TextResource implements RequestHandler  {
    public FourOhFour() {
        super(404, "Four, oh, four.  Resource not found.");
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return this;
    }
}
