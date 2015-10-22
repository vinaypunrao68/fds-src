/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.policy;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.svc.types.FDSP_PolicyInfoType;
import com.formationds.apis.ConfigurationService;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.apache.commons.io.IOUtils;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class PostQoSPolicy
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PostQoSPolicy.class );

  private final ConfigurationService.Iface configApi;

  public PostQoSPolicy( final ConfigurationService.Iface configApi ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      logger.debug( "Creating QoS policy." );

      String policy_name = "";
      long iops_min = 0;
      long iops_max = 0;
      int rel_prio = 0;

      FDSP_PolicyInfoType qosPolicy = new FDSP_PolicyInfoType();

      String source = IOUtils.toString(request.getInputStream());
      try {
          JSONObject o = new JSONObject(source);
          policy_name = o.getString( "policy_name" );
          iops_min = o.getLong( "iops_min" );
          iops_max = o.getLong( "iops_max" );
          rel_prio = o.getInt( "rel_prio" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing information to create QoS Policy. Policy name, min or max IOPS, or relative priority." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      logger.debug( "Creating QoS policy {} with min/max IOPS {}/{} and relative priority {}.",
              policy_name, iops_min, iops_max, rel_prio );

      try {
          qosPolicy = configApi.createQoSPolicy(policy_name, iops_min, iops_max, rel_prio);
      } catch( ApiException e ) {

          if ( e.getErrorCode().equals(ErrorCode.RESOURCE_ALREADY_EXISTS)) {
              JSONObject o = new JSONObject();
              o.put("status", "already_exists");
              o.put("policy_name", policy_name);
              o.put("policy_id", qosPolicy.getPolicy_id());
              return new JsonResource(o);
          }

          logger.error( "POST::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      } catch ( TException | SecurityException se ) {
          logger.error( "POST::FAILED::" + se.getMessage(), se );

          // allow dispatcher to handle
          throw se;
      }

      if( qosPolicy.getPolicy_id() > 0 ) {
          JSONObject o = new JSONObject();
          o.put("status", "success");
          o.put("policy_name",  qosPolicy.getPolicy_name());
          o.put("policy_id",  qosPolicy.getPolicy_id());
          o.put("iops_min",  qosPolicy.getIops_assured());
          o.put("iops_max",  qosPolicy.getIops_throttle());
          o.put("rel_prio",  qosPolicy.getRel_prio());
          return new JsonResource(o);
      }

      throw new Exception( "No policy id after createQoSPolicy call!" );
  }
}

