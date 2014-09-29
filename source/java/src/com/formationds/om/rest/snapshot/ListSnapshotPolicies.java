/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListSnapshotPolicies implements RequestHandler {

  private ConfigurationApi config;

    public ListSnapshotPolicies(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
        final long unused = 0L;

        final ObjectMapper mapper = new ObjectMapper();
        final List<SnapshotPolicy> policies = new ArrayList<>();

      final List<com.formationds.apis.SnapshotPolicy> _policies =
          config.listSnapshotPolicies( unused );
        if (_policies == null || _policies.isEmpty()) {
          return new JsonResource(new JSONArray(policies));
        }

        for (final com.formationds.apis.SnapshotPolicy policy : _policies) {
          final SnapshotPolicy modelPolicy = new SnapshotPolicy();

          modelPolicy.setId(policy.getId());
          modelPolicy.setName(policy.getPolicyName());
System.out.println( "::RRULE::" +  policy.getRecurrenceRule() );
          modelPolicy.setRecurrenceRule( policy.getRecurrenceRule() );
          modelPolicy.setRetention(policy.getRetentionTimeSeconds());

          policies.add(modelPolicy);
        }

        return new JsonResource(new JSONArray(mapper.writeValueAsString(policies)));
    }
}
