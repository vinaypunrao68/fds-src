/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeleteSnapshotPolicy implements RequestHandler {

    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private ConfigurationApi config;

    public DeleteSnapshotPolicy(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {
        if (!FdsFeatureToggles.USE_CANNED.isActive()) {
            config.deleteSnapshotPolicy(
                    requiredLong(routeParameters, REQ_PARAM_POLICY_ID));
        }

        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
