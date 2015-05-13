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

public class PutLocalDomainServices
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PutLocalDomainServices.class );

  private final Authorizer authorizer;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public PutLocalDomainServices( final Authorizer authorizer,
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
      boolean sm = false;
      boolean dm = false;
      boolean am = false;
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
          sm = o.getBoolean( "sm" );
          dm = o.getBoolean( "dm" );
          am = o.getBoolean( "am" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain action to PUT." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }
      
      try {
          JSONObject o = new JSONObject(source);
          sm = o.getBoolean( "sm" );
          dm = o.getBoolean( "dm" );
          am = o.getBoolean( "am" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing Local Domain Services to PUT." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }
      
      logger.debug( "{} services for Local Domain {}. SM: {}; DM: {}; AM: {}.", action, domainName, sm, dm, am );

      try {
          if (action.equals("activate")) {
              configApi.activateLocalDomainServices(domainName, sm, dm, am);
          } else if (action.equals("remove")) {
              configApi.removeLocalDomainServices(domainName, sm, dm, am);
          }
      } catch( Exception e ) {
          logger.error( "PUT::{}::FAILED::" + e.getMessage(), action, e );

          // allow dispatcher to handle
          throw e;
      }

      return new JsonResource(new JSONObject().put("status", "OK"));
  }
}

