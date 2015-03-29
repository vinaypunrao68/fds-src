/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.domain;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.apis.ConfigurationService;
import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.commons.model.Domain;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.ConnectorType;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.SizeUnit;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.web.toolkit.UsageException;

import org.apache.commons.io.IOUtils;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.json.JSONException;
import org.json.JSONArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

public class PutLocalDomainServices
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( PutLocalDomainServices.class );

  private final Authorizer authorizer;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public PutLocalDomainServices( final Authorizer authorizer,
                                 final FDSP_ConfigPathReq.Iface legacyConfigPath,
                                 final ConfigurationService.Iface configApi,
                                 final AuthenticationToken token ) {
    this.authorizer = authorizer;
    this.legacyConfigPath = legacyConfigPath;
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

