
/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.formationds.commons.model.helper.VolumeId;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

public class AttachSnapshotPolicyIdToVolumeId implements RequestHandler {
  private static final Logger LOG =
    Logger.getLogger(AttachSnapshotPolicyIdToVolumeId.class);

    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private ConfigurationApi config;

    public AttachSnapshotPolicyIdToVolumeId(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters)
            throws Exception {
      final long policyId = requiredLong( routeParameters, REQ_PARAM_POLICY_ID );

      try(final Reader reader =
            new InputStreamReader( request.getInputStream(), "UTF-8")) {
        Gson gson = new GsonBuilder().create();
        final VolumeId volumeId = gson.fromJson( reader, VolumeId.class);

        LOG.trace( "ATTACH:: VOLUME ID: " + volumeId.getVolumeId() +
                   " POLICY ID: " + policyId );
        config.attachSnapshotPolicy( volumeId.getVolumeId(), policyId );
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
