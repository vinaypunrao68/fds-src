package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class GetThrottle implements RequestHandler {
    private TransientState state;
    private Throttle throttle;

    public GetThrottle(TransientState state, Throttle throttle) {
        this.state = state;
        this.throttle = throttle;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return new JsonResource(new JSONObject()
                .put("value", state.getThrottleValue(throttle))
                .put("unit", "IO operations/s"));
    }
}
