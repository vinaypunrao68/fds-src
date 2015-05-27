/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest.v08.volumes;

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeleteVolume
  implements RequestHandler {
  private Authorizer authorizer;
  private ConfigurationApi    config;
  private AuthenticationToken token;

  public DeleteVolume(ConfigurationApi config, Authorizer authorizer, AuthenticationToken token) {
    this.authorizer = authorizer;
    this.config = config;
    this.token = token;
  }

  @Override
  public Resource handle(Request request, Map<String, String> routeParameters)
      throws Exception {

    return new JsonResource(new JSONObject().put("status", "OK"));
  }
}
