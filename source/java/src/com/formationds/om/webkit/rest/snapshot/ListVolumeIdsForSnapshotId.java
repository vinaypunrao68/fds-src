/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.webkit.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListVolumeIdsForSnapshotId
  implements RequestHandler {

  private static final String REQ_PARAM_POLICY_ID = "policyId";
  private ConfigurationApi config;

  public ListVolumeIdsForSnapshotId(final ConfigurationApi config) {
    this.config = config;
  }

  @Override
  public Resource handle(final Request request,
                         final Map<String, String> routeParameters)
      throws Exception {
    final ObjectMapper mapper = new ObjectMapper();
    final List<Long> volumeIds = new ArrayList<>();
    final long policyId = requiredLong(routeParameters,
                                       REQ_PARAM_POLICY_ID);

    volumeIds.addAll(config.listVolumesForSnapshotPolicy(policyId));
    if (volumeIds.isEmpty()) {
      return new JsonResource(new JSONArray(volumeIds));
    }

    return new JsonResource(new JSONArray(mapper.writeValueAsString(volumeIds) ) );
  }
}
