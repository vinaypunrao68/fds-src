package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class SetCurrentSearch implements RequestHandler {
    private TransientState state;

    public SetCurrentSearch(TransientState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String q = request.getParameter("q");
        state.setSearchExpression(q);
        return new TextResource(q);
    }
}
