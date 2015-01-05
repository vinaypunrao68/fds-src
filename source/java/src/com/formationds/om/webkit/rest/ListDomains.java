package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Map;

public class ListDomains implements RequestHandler {
    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray();
        array.put(new JSONObject()
                .put("site", "Fremont")
                .put("domain", "Formation Data Systems")
                .put("id", 0));

        array.put(new JSONObject()
                .put("site", "San Francisco")
                .put("domain", "Formation Data Systems")
                .put("id", 1));

        array.put(new JSONObject()
                .put("site", "Boulder")
                .put("domain", "QuickLeft, Inc.")
                .put("id", 2));

        return new JsonResource(array);
    }
}
