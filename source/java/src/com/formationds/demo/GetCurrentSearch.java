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

public class GetCurrentSearch implements RequestHandler {
    private DemoState state;

    public GetCurrentSearch(DemoState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return state.getCurrentSearchExpression()
                .map(s -> (Resource) new JsonResource(new JSONObject().put("q", s)))
                .orElse(new FourOhFour());
    }
}
