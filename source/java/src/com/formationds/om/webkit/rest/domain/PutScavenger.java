/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.domain;

import com.formationds.apis.ConfigurationService;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.UsageException;

import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class PutScavenger
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PutScavenger.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public PutScavenger( final Authorizer authorizer,
                       final ConfigurationService.Iface configApi,
                       final AuthenticationToken token ) {
    this.authorizer = authorizer;
    this.configApi = configApi;
    this.token = token;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      String domainName = "";
      String scavengerAction = "";
      
      try {
          domainName = requiredString(routeParameters, "local_domain");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      /**
       * What scavenger action do we have?
       */
      String source = IOUtils.toString(request.getInputStream());
      try {
          JSONObject o = new JSONObject(source);
          scavengerAction = o.getString( "action" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing scavenger action to PUT." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }
      
      logger.debug( "Set scavenger action {} for Local Domain {}.", scavengerAction, domainName );

      try {
          configApi.setScavenger(domainName, scavengerAction);
      } catch( Exception e ) {
          logger.error( "PUT::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
  }
}

