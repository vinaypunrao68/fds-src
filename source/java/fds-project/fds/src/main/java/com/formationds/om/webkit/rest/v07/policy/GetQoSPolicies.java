/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.policy;

import com.formationds.protocol.FDSP_PolicyInfoType;
import com.formationds.apis.ConfigurationService;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Map;
import java.util.List;

public class GetQoSPolicies
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( GetQoSPolicies.class );

  private final ConfigurationService.Iface configApi;

  public GetQoSPolicies( final ConfigurationService.Iface configApi ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      List<FDSP_PolicyInfoType> qosPolicies;
      
      logger.debug( "Listing QoS Policies." );

      try {
          qosPolicies = configApi.listQoSPolicies(0);
      } catch( Exception e ) {
          logger.error( "GET::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }
      
      JSONArray array = new JSONArray();

      for (FDSP_PolicyInfoType qosPolicy : qosPolicies) {
          array.put(new JSONObject()
                          .put("policy_name", qosPolicy.getPolicy_name())
                          .put("policy_id", qosPolicy.getPolicy_id())
                          .put("iops_min", qosPolicy.getIops_assured())
                          .put("iops_max", qosPolicy.getIops_throttle())
                          .put("rel_prio", qosPolicy.getRel_prio()));
      }

      return new JsonResource(array);
  }
}

