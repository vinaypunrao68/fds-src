package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class ShowGlobalDomain implements RequestHandler {
    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        return new JsonResource(new JSONObject().put("name", "FDS"));
    }
}
