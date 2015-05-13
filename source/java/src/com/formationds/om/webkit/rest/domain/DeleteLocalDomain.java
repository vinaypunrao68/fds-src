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

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.util.Map;

public class DeleteLocalDomain
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( DeleteLocalDomain.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public DeleteLocalDomain( final Authorizer authorizer,
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
      
      try {
          domainName = requiredString(routeParameters, "local_domain");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      logger.debug( "Delete Local Domain {}.", domainName );

      try {
          configApi.deleteLocalDomain(domainName);
      } catch( Exception e ) {
          logger.error( "DELETE::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
  }
}

