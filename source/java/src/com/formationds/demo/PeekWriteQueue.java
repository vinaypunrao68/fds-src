package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.FourOhFour;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;
import java.util.Optional;

public class PeekWriteQueue implements RequestHandler {
    private DemoState state;

    public PeekWriteQueue(DemoState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        Optional<ImageResource> resource = state.peekWriteQueue();
        if (resource.isPresent()) {
            return new JsonResource(new JSONObject().put("url", resource.get().getThumbnailUrl()));
        } else {
            return new FourOhFour();
        }
    }
}
