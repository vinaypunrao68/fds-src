
/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.commons.model.Status;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class AttachSnapshotPolicyIdToVolumeId implements RequestHandler {
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
        config.attachSnapshotPolicy(
                requiredLong(routeParameters, REQ_PARAM_VOLUME_ID),
                requiredLong(routeParameters, REQ_PARAM_POLICY_ID));

        return new JsonResource(new Status(HttpResponseStatus.OK));
    }
}
