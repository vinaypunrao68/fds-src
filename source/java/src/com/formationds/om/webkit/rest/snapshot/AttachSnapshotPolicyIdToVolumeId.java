
/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.webkit.rest.snapshot;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class AttachSnapshotPolicyIdToVolumeId implements RequestHandler {
    private static final Logger logger =
      Logger.getLogger(AttachSnapshotPolicyIdToVolumeId.class);

    private static final String REQ_PARAM_VOLUME_ID = "volumeId";
    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private com.formationds.util.thrift.ConfigurationApi config;

    public AttachSnapshotPolicyIdToVolumeId(final com.formationds.util.thrift.ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
        throws Exception {

        routeParameters.forEach((key, value) -> {
            logger.trace("KEY::" + key + " VALUE::'" + value + "'");
        });

        final long volumeId = requiredLong(routeParameters, REQ_PARAM_VOLUME_ID);
        final long policyId = requiredLong(routeParameters, REQ_PARAM_POLICY_ID);

        logger.trace("ATTACH:: VOLUME ID: " + volumeId + " POLICY ID: " + policyId);
        config.attachSnapshotPolicy(volumeId, policyId);

        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
