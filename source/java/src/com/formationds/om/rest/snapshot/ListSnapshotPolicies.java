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
import net.fortuna.ical4j.model.Recur;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListSnapshotPolicies implements RequestHandler {

    private final long unused = 0L;
    private ConfigurationApi config;

    public ListSnapshotPolicies(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {

        final ObjectMapper mapper = new ObjectMapper();
        final List<SnapshotPolicy> policies = new ArrayList<>();

        if (FdsFeatureToggles.USE_CANNED.isActive()) {
            for (int i = 1; i <= 10; i++) {
                final SnapshotPolicy policy = new SnapshotPolicy();

                policy.setId(i);
                policy.setName(String.format("snapshot policy name %s", i));
                policy.setRecurrenceRule(new Recur("FREQ=DAILY;UNTIL=19971224T000000Z"));
                policy.setRetention(System.currentTimeMillis() / 1000);

                policies.add(policy);
            }
        } else {
            final List<com.formationds.apis.SnapshotPolicy> _policies =
                    config.listSnapshotPolicies(unused);
            if (_policies == null || _policies.isEmpty()) {
                return new JsonResource(new JSONObject(new Status(HttpResponseStatus.NO_CONTENT)));
            }

            for (final com.formationds.apis.SnapshotPolicy policy : _policies) {
                final SnapshotPolicy modelPolicy = new SnapshotPolicy();

                modelPolicy.setId(policy.getId());
                modelPolicy.setName(policy.getPolicyName());
                modelPolicy.setRecurrenceRule(new Recur(policy.getRecurrenceRule()));
                modelPolicy.setRetention(policy.getRetentionTimeSeconds());

                policies.add(modelPolicy);
            }
        }

        return new JsonResource(new JSONArray(mapper.writeValueAsString(policies)));
    }
}
