/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

package com.formationds.om.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

<<<<<<< HEAD
/**
 * @author ptinius
 */
public class ListVolumeIdsForSnapshotId
        extends OMRestBase {
  private static final Logger LOG =
          Logger.getLogger(ListVolumeIdsForSnapshotId.class);

  private static final String REQ_PARAM_POLICY_ID = "policyId";

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  public ListVolumeIdsForSnapshotId(final ConfigurationServiceCache config) {
    super(config);
  }

  /**
   * @param request         the {@link Request}
   * @param routeParameters the {@link Map} of route parameters
   * @return Returns the {@link Resource}
   * @throws Exception any unhandled error
   */
  @Override
  public Resource handle(final Request request,
                         final Map<String, String> routeParameters)
          throws Exception {
    final ObjectMapper mapper = new ObjectMapper();
    final List<Long> volumeIds = new ArrayList<>();
    final long policyId = requiredLong(routeParameters,
            REQ_PARAM_POLICY_ID);

    if (FdsFeatureToggles.USE_CANNED.isActive()) {
      for (int i = 1; i <= 10; i++) {
        volumeIds.add((long) i);
      }
    } else {
      volumeIds.addAll(
              getConfigurationServiceCache().listVolumesForSnapshotPolicy(policyId));
      if (volumeIds.isEmpty()) {
        final Status status = ObjectFactory.createStatus();
        status.setStatus(HttpResponseStatus.NO_CONTENT.reasonPhrase());
        status.setCode(HttpResponseStatus.NO_CONTENT.code());

        return new JsonResource(new JSONObject(mapper.writeValueAsString(status)));
      }
    }

    return new JsonResource(new JSONArray(mapper.writeValueAsString(volumeIds)));
  }
=======
public class ListVolumeIdsForSnapshotId implements RequestHandler {

    private static final String REQ_PARAM_POLICY_ID = "policyId";
    private ConfigurationApi config;

    public ListVolumeIdsForSnapshotId(final ConfigurationApi config) {
        this.config = config;
    }

    @Override
    public Resource handle(final Request request,
                           final Map<String, String> routeParameters) throws Exception {
        final ObjectMapper mapper = new ObjectMapper();
        final List<Long> volumeIds = new ArrayList<>();
        final long policyId = requiredLong(routeParameters,
                REQ_PARAM_POLICY_ID);

        if (FdsFeatureToggles.USE_CANNED.isActive()) {
            for (int i = 1; i <= 10; i++) {
                volumeIds.add((long) i);
            }
        } else {
            volumeIds.addAll(config.listVolumesForSnapshotPolicy(policyId));
            if (volumeIds.isEmpty()) {
                return new JsonResource(new  JSONArray( volumeIds ) );
            }
        }

        return new JsonResource(new JSONArray(mapper.writeValueAsString(volumeIds)));
    }
>>>>>>> dev
}
