/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest;

import com.formationds.apis.ConfigurationService;
import com.formationds.commons.model.Stream;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class DeregisterStream
  implements RequestHandler {
  private ConfigurationService.Iface configApi;

  public DeregisterStream( ConfigurationService.Iface configApi ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {
    String source = IOUtils.toString( request.getInputStream() );
    final Stream stream = ObjectModelHelper.toObject( source, Stream.class );

    configApi.deregisterStream( stream.getId() );
    return new JsonResource(new JSONObject().put("status", "OK"));
  }
}
