/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.om.rest.snapshot;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.formationds.commons.model.ObjectFactory;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.rest.OMRestBase;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationServiceCache;
import net.fortuna.ical4j.model.Recur;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class ListSnapshotPolicies
        extends OMRestBase {
  private static final Logger LOG =
          Logger.getLogger(ListSnapshotPolicies.class);

  private final long unused = 0L;

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  public ListSnapshotPolicies(final ConfigurationServiceCache config) {
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
    final List<SnapshotPolicy> policies = new ArrayList<>();

    if (FdsFeatureToggles.USE_CANNED.isActive()) {
      for (int i = 1; i <= 10; i++) {
        final SnapshotPolicy policy = ObjectFactory.createSnapshotPolicy();

        policy.setId(i);
        policy.setName(String.format("snapshot policy name %s", i));
        policy.setRecurrenceRule(new Recur("FREQ=DAILY;UNTIL=19971224T000000Z"));
        policy.setRetention(System.currentTimeMillis() / 1000);

        policies.add(policy);
      }
    } else {
      final List<com.formationds.apis.SnapshotPolicy> _policies =
              getConfigurationServiceCache().listSnapshotPolicies(unused);
      if (_policies == null || _policies.isEmpty()) {
        return new JsonResource(new JSONObject(mapper.writeValueAsString(noContent())));
      }

      for (final com.formationds.apis.SnapshotPolicy policy : _policies) {
        final SnapshotPolicy modelPolicy = ObjectFactory.createSnapshotPolicy();

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
