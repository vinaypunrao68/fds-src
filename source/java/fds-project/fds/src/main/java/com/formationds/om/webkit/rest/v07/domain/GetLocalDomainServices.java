/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.domain;

import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.apis.ConfigurationService;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.UsageException;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.List;
import java.util.Map;

public class GetLocalDomainServices
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( GetLocalDomainServices.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public GetLocalDomainServices( final Authorizer authorizer,
                            final ConfigurationService.Iface configApi,
                            final AuthenticationToken token ) {
    this.authorizer = authorizer;
    this.configApi = configApi;
    this.token = token;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      List<FDSP_Node_Info_Type> Services;

      String domainName = "";
      
      try {
          domainName = requiredString(routeParameters, "local_domain");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }
      
      logger.debug( "Listing services for Local Domain {}.", domainName );

      try {
          Services = configApi.listLocalDomainServices(domainName);
      } catch( Exception e ) {
          logger.error( "GET::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }
      
      JSONArray array = new JSONArray();

      for (FDSP_Node_Info_Type service : Services) {
          array.put(new JSONObject()
                          .put("node_id", service.getNode_id())
                          .put("node_state", service.getNode_state())
                          .put("node_type", service.getNode_type())
                          .put("node_name", service.getNode_name())
                          .put("ip_hi_addr", service.getIp_hi_addr())
                          .put("ip_lo_addr", service.getIp_lo_addr())
                          .put("control_port", service.getControl_port())
                          .put("data_port", service.getData_port())
                          .put("migration_port", service.getMigration_port())
                          .put("node_uuid", service.getNode_uuid())
                          .put("service_uuid", service.getService_uuid())
                          .put("node_root", service.getNode_root())
                          .put("metasync_port", service.getMetasync_port())
                          );
      }

      return new JsonResource(array);
  }
}

