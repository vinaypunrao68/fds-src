package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

public class ListDomains implements RequestHandler {
    @Override
    public Resource handle(Request request) throws Exception {
        JSONObject o = new JSONObject()
                .put("site", "Fremont")
                .put("domain", "Formation Data Systems")
                .put("id", 0);
        JSONArray array = new JSONArray().put(o);
        return new JsonResource(array);
    }
}
