/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om.webkit.rest.policy;

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

