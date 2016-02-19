/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.policy;

import com.formationds.apis.ConfigurationService;
import com.formationds.protocol.svc.types.FDSP_PolicyInfoType;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.UsageException;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class PutQoSPolicy
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PutQoSPolicy.class );

  private final ConfigurationService.Iface configApi;

  public PutQoSPolicy(final ConfigurationService.Iface configApi) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      String current_policy_name = "";
      String new_policy_name = "";
      long iops_min = 0;
      long iops_max = 0;
      int rel_prio = 0;
      
      FDSP_PolicyInfoType qosPolicy = new FDSP_PolicyInfoType();

      try {
          current_policy_name = requiredString(routeParameters, "current_policy_name");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing current QoS Policy name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      String source = IOUtils.toString(request.getInputStream());
      try {
          JSONObject o = new JSONObject(source);
          new_policy_name = o.getString( "new_policy_name" );
          iops_min = o.getLong("iops_min");
          iops_max = o.getLong("iops_max");
          rel_prio = o.getInt("rel_prio");
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing information to create QoS Policy. Policy name, min or max IOPS, or relative priority." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      logger.debug( "Modifying QoS policy {} with (new?) name {}, min/max IOPS {}/{}, and relative priority {}.",
              current_policy_name, new_policy_name, iops_min, iops_max, rel_prio );

      try {
          qosPolicy = configApi.modifyQoSPolicy(current_policy_name, new_policy_name, iops_min, iops_max, rel_prio);
      } catch( Exception e ) {

          logger.error( "POST::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }

      if( qosPolicy.getPolicy_id() > 0 ) {
          JSONObject o = new JSONObject();
          o.put("status", "success");
          o.put("policy_name", qosPolicy.getPolicy_name());
          o.put("policy_id", qosPolicy.getPolicy_id());
          o.put("iops_min", qosPolicy.getIops_assured());
          o.put("iops_max", qosPolicy.getIops_throttle());
          o.put("rel_prio", qosPolicy.getRel_prio());
          return new JsonResource(o);
      }

      throw new Exception( "No policy id after modifyQoSPolicy call!" );
  }
}

