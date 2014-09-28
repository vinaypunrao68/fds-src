/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.builder.SnapshotPolicyBuilder;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class CreateSnapshotPolicy
  implements RequestHandler {
    private ConfigurationApi config;

    public CreateSnapshotPolicy(ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
      long policyId;
      try(final Reader reader =
            new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {
        Gson gson = new GsonBuilder().create();
        final SnapshotPolicy policy = gson.fromJson( reader,
                                                     SnapshotPolicy.class );

        policyId = config.createSnapshotPolicy( policy.getName(),
                                                policy.getRecurrenceRule().toString(),
                                                policy.getRetention());
      }

      final SnapshotPolicy snapshotPolicy =
        new SnapshotPolicyBuilder().withId( policyId ).build();
      return new JsonResource( new JSONObject( snapshotPolicy ) );
    }
}
