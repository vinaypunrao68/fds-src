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
import org.json.JSONException;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class PutLocalDomain
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PutLocalDomain.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public PutLocalDomain( final Authorizer authorizer,
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
      String action = "";
      
      try {
          domainName = requiredString(routeParameters, "local_domain");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      /**
       * What PUT action do we have?
       */
      String source = IOUtils.toString(request.getInputStream());
      try {
          JSONObject o = new JSONObject(source);
          action = o.getString( "action" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain action to PUT." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      try {
          if (action.equals("rename")) {
              // Look up the new name.
              String newDomainName = "";
              try {
                  JSONObject o = new JSONObject(source);
                  newDomainName = o.getString( "new_domain_name" );
              } catch(JSONException e) {
                  return new JsonResource(
                          new JSONObject().put( "message", "Missing new Local Domain name." ),
                          HttpServletResponse.SC_BAD_REQUEST );
              }

              logger.debug( "Rename Local Domain {} to {}.", domainName, newDomainName );
              configApi.updateLocalDomainName(domainName, newDomainName);
          } else if (action.equals("change_site")) {
              // Look up the new site name.
              String newSiteName = "";
              try {
                  JSONObject o = new JSONObject(source);
                  newSiteName = o.getString( "new_site_name" );
              } catch(JSONException e) {
                  return new JsonResource(
                          new JSONObject().put( "message", "Missing new site name." ),
                          HttpServletResponse.SC_BAD_REQUEST );
              }

              logger.debug( "Rename Local Domain {} site to {}.", domainName, newSiteName );
              configApi.updateLocalDomainSite(domainName, newSiteName);
          } else if (action.equals("startup")) {
              logger.debug( "Startup Local Domain {}.", domainName );
              configApi.startupLocalDomain(domainName);
          } else if (action.equals("shutdown")) {
              logger.debug( "Shutdown Local Domain {}.", domainName );
              configApi.shutdownLocalDomain(domainName);
          }
      } catch( Exception e ) {
          logger.error( "PUT::{}::FAILED::" + e.getMessage(), action, e );

          // allow dispatcher to handle
          throw e;
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
  }
}

