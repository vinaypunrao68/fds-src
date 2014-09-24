/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CreateSnapshot implements RequestHandler {
    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters) throws Exception {
        return new JsonResource(new JSONObject().put("status", "NOT_IMPLEMENTED"));
    }
}
