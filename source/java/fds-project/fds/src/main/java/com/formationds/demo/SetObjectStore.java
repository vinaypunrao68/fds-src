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

public class SetObjectStore implements RequestHandler {
    private DemoState state;

    public SetObjectStore(DemoState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        ObjectStoreType type = requiredEnum(request, "type", ObjectStoreType.class);
        state.setObjectStore(type);
        return new JsonResource(new JSONObject().put("type", type.toString()));
    }
}
