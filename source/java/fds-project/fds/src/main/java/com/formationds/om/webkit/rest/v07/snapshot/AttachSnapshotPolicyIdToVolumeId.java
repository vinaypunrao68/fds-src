
/**
 * Copyright (c) 2015 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.webkit.rest.v07.snapshot;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class AttachSnapshotPolicyIdToVolumeId implements RequestHandler {
    private static final Logger logger = LogManager.getLogger(AttachSnapshotPolicyIdToVolumeId.class);

    private static final String REQ_PARAM_VOLUME_ID = "volumeId";
    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private ConfigurationApi config;

    public AttachSnapshotPolicyIdToVolumeId(final ConfigurationApi config) {
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
