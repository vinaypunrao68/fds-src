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
import com.formationds.om.rest.OMRestBase;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationServiceCache;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;

public class DeleteSnapshotForVolume
        extends OMRestBase {
  private static final Logger LOG =
          LoggerFactory.getLogger(DeleteSnapshotForVolume.class);

  private static final String REQ_PARAM_VOLUME_ID = "volumeId";
  private static final String REQ_PARAM_POLICY_ID = "policyId";

  /**
   * @param config the {@link com.formationds.xdi.ConfigurationServiceCache}
   */
  public DeleteSnapshotForVolume(final ConfigurationServiceCache config) {
    super(config);
  }

  /**
   * @param request         the {@link Request}
   * @param routeParameters the {@link Map} of route parameters
   * @return Returns the {@link Resource}
   * @throws Exception any unhandled error
   */
  @Override
  public Resource handle(Request request, Map<String, String> routeParameters)
          throws Exception {
    final ObjectMapper mapper = new ObjectMapper();

    // TODO need iface definition for deleteSnapshotForVolume
//      getConfigurationServiceCache().deleteSnapshotForVolume(
//        requiredLong( routeParameters, REQ_PARAM_VOLUME_ID ),
//        requiredLong( routeParameters, REQ_PARAM_POLICY_ID ) );

    return new JsonResource(new JSONObject(mapper.writeValueAsString(ok())));
  }
}
