/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.domain;

import com.formationds.apis.ConfigurationService;
import com.formationds.client.v08.converters.ExternalModelConverter;
import com.formationds.client.v08.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ListLocalDomains
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( ListLocalDomains.class );

  private final ConfigurationService.Iface configApi;

  public ListLocalDomains( final Authorizer authorizer,
                          final ConfigurationService.Iface configApi,
                          final AuthenticationToken token ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
      throws Exception {

      List<com.formationds.apis.LocalDomain> localDomains;
      
      logger.debug( "Listing local domains." );

      try {
          localDomains = configApi.listLocalDomains(0);
      } catch( Exception e ) {
          logger.error( "GET::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }
      
      List<Domain> domains = new ArrayList<Domain>();
      
      for ( com.formationds.apis.LocalDomain thriftDomain : localDomains ){
    	  Domain domain = ExternalModelConverter.convertToExternalDomain( thriftDomain );
    	  domains.add( domain );
      }

      String jsonString = ObjectModelHelper.toJSON( domains );

      return new TextResource( jsonString );
  }
}

