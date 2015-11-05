/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v07.domain;

import com.formationds.apis.ConfigurationService;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.Map;

public class GetLocalDomains
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( GetLocalDomains.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public GetLocalDomains( final Authorizer authorizer,
                          final ConfigurationService.Iface configApi,
                          final AuthenticationToken token ) {
    this.authorizer = authorizer;
    this.configApi = configApi;
    this.token = token;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      List<com.formationds.apis.LocalDomainDescriptorV07> localDomains;
      
      logger.debug( "Listing local domains." );

      try {
          localDomains = configApi.listLocalDomainsV07(0);
      } catch( Exception e ) {
          logger.error( "GET::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }
      
      JSONArray array = new JSONArray();

      for (com.formationds.apis.LocalDomainDescriptorV07 localDomain : localDomains) {
          array.put(new JSONObject()
                          .put("name", localDomain.getName())
                          .put("id", localDomain.getId())
                          .put("site", localDomain.getSite()));
      }

      return new JsonResource(array);
  }
}

