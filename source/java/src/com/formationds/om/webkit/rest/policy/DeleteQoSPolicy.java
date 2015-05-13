/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.policy;

import com.formationds.apis.ConfigurationService;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.UsageException;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class DeleteQoSPolicy
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( DeleteQoSPolicy.class );

  private final ConfigurationService.Iface configApi;

  public DeleteQoSPolicy( final ConfigurationService.Iface configApi ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      String policy_name = "";
      
      try {
          policy_name = requiredString(routeParameters, "policy_name");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing QoS Policy name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      logger.debug( "Delete QoS Policy {}.", policy_name );

      try {
          configApi.deleteQoSPolicy(policy_name);
      } catch( Exception e ) {
          logger.error( "DELETE::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
  }
}

