/**
 * Copyright (c) 2015 Formation Data Systems. All rights reserved.
 */

package com.formationds.om.webkit.rest.snapshot;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class RestoreSnapshot implements RequestHandler {
    private static final String REQ_PARAM_VOLUME_ID = "volumeId";
    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private ConfigurationApi config;

    public RestoreSnapshot(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
        throws Exception {

        config.restoreClone(
                               requiredLong(routeParameters, REQ_PARAM_VOLUME_ID),
                               requiredLong(routeParameters, REQ_PARAM_POLICY_ID));

        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
