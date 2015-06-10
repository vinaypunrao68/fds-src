package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class SetThrottle implements RequestHandler {
    private DemoState state;
    private Throttle throttle;

    public SetThrottle(DemoState state, Throttle throttle) {
        this.state = state;
        this.throttle = throttle;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        int value = requiredInt(routeParameters, "value");
        if (throttle == Throttle.read) {
            state.setReadThrottle(value);
        } else if (throttle == Throttle.write) {
            state.setWriteThrottle(value);
        }
        return new TextResource(Integer.toString(value));
    }
}
