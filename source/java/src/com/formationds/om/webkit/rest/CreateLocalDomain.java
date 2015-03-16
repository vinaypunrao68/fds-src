/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;

import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;

public class CreateLocalDomain
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( CreateLocalDomain.class );

  private final Authorizer authorizer;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public CreateLocalDomain( final Authorizer authorizer,
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
      long domainId = -1;
      
      try {
          domainName = requiredString(routeParameters, "local_domain");
      } catch(UsageException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing new Local Domain name." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      String source = IOUtils.toString(request.getInputStream());
      JSONObject o = new JSONObject(source);
      try {
          domainId = o.getLong( "id" );
      } catch(JSONException e) {
          return new JsonResource(
                  new JSONObject().put( "message", "Missing new Local Domain ID." ),
                  HttpServletResponse.SC_BAD_REQUEST );
      }

      logger.debug( "Creating local domain {} with ID {}", domainName, domainId );

      try {
          domainId = configApi.createLocalDomain(domainName, domainId);
      } catch( ApiException e ) {

          if ( e.getErrorCode().equals(ErrorCode.RESOURCE_ALREADY_EXISTS)) {
              return new TextResource(domainName);
          }

          logger.error( "CREATE::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      } catch ( TException | SecurityException se ) {
          logger.error( "CREATE::FAILED::" + se.getMessage(), se );

          // allow dispatcher to handle
          throw se;
      }

      if( domainId > 0 ) {
          return new JsonResource(new JSONObject().put("domainId", domainId));
      }

      throw new Exception( "no domain id after createLocalDomain call!" );
  }
}

