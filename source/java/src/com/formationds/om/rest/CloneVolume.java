/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CloneVolume
  implements RequestHandler {

  private static final String REQ_PARAM_VOLUME_ID = "volumeId";
  private static final String REQ_PARAM_POLICY_ID = "policyId";
  private static final String REQ_PARAM_CLONED_VOLUME_NAME = "clonedVolumeName";
  private ConfigurationApi config;

  public CloneVolume( final ConfigurationApi config ) {
    this.config = config;
  }

  @Override
  public Resource handle( final Request request,
                          final Map<String, String> routeParameters )
    throws Exception {
    final long clonedVolumeId = config.cloneVolume(
      requiredLong( routeParameters, REQ_PARAM_VOLUME_ID ),
      requiredLong( routeParameters, REQ_PARAM_POLICY_ID ),
      requiredString( routeParameters, REQ_PARAM_CLONED_VOLUME_NAME ) );

    return new JsonResource( new JSONObject( clonedVolumeId ) );
  }
}
