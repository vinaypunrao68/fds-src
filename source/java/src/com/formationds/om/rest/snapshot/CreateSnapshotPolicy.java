/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.Status;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CreateSnapshotPolicy implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(CreateSnapshotPolicy.class);

    private ConfigurationApi config;

    public CreateSnapshotPolicy(ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
        final ObjectMapper mapper = new ObjectMapper();

        if (!FdsFeatureToggles.USE_CANNED.isActive()) {
            final SnapshotPolicy policy = mapper.readValue(request.getInputStream(), SnapshotPolicy.class);

            LOG.trace("calling XDI create snapshot policy with " + policy);
            config.createSnapshotPolicy(
                    policy.getName(),
                    policy.getRecurrenceRule().toString(),
                    policy.getRetention());
        }

        return new JsonResource(new Status(HttpResponseStatus.OK));
    }
}
