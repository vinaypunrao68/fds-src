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

public class DeleteSnapshotForVolume implements RequestHandler {

    private static final String REQ_PARAM_VOLUME_ID = "volumeId";
    private static final String REQ_PARAM_POLICY_ID = "policyId";

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

        // TODO need iface definition for deleteSnapshotForVolume
//      getConfigurationServiceCache().deleteSnapshotForVolume(
//        requiredLong( routeParameters, REQ_PARAM_VOLUME_ID ),
//        requiredLong( routeParameters, REQ_PARAM_POLICY_ID ) );

        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
