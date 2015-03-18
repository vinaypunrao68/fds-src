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

public class ListLocalDomains
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( ListLocalDomains.class );

  private final Authorizer authorizer;
  private final FDSP_ConfigPathReq.Iface legacyConfigPath;
  private final ConfigurationService.Iface configApi;
  private final AuthenticationToken token;

  public ListLocalDomains( final Authorizer authorizer,
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

      List<com.formationds.apis.LocalDomain> localDomains;
      
      logger.debug( "Listing local domains." );

      try {
          localDomains = configApi.listLocalDomains(0);
      } catch( Exception e ) {
          logger.error( "LIST::FAILED::" + e.getMessage(), e );

          // allow dispatcher to handle
          throw e;
      }
      
      JSONArray array = new JSONArray();

      for (com.formationds.apis.LocalDomain localDomain : localDomains) {
          array.put(new JSONObject()
                          .put("name", localDomain.name)
                          .put("id", localDomain.id)
                          .put("site", localDomain.site));
      }

      return new JsonResource(array);
  }
}

