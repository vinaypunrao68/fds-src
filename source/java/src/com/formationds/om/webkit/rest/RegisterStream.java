/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.webkit.rest;

import com.formationds.apis.ConfigurationService;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.commons.io.IOUtils;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class RegisterStream
  implements RequestHandler {
  private ConfigurationService.Iface configApi;

  public RegisterStream( ConfigurationService.Iface configApi ) {
    this.configApi = configApi;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {
    String source = IOUtils.toString( request.getInputStream() );
    JSONObject o = new JSONObject( source );
    String url = o.getString( "url" );
    String httpMethod = o.getString( "method" );
    List<String> volumeNames = asList( o.getJSONArray( "volumes" ) );

    int sampleFreqSeconds = o.getInt( "frequency" );
    int durationSeconds = o.getInt( "duration" );

    int id = configApi.registerStream( url, httpMethod, volumeNames, sampleFreqSeconds, durationSeconds );
    JSONObject result = new JSONObject().put( "id", id );
    return new JsonResource( result );
  }

  private List<String> asList( JSONArray jsonArray ) {
    ArrayList<String> result = new ArrayList<>();
    for( int i = 0;
         i < jsonArray.length();
         i++ ) {
      result.add( jsonArray.getString( i ) );
    }
    return result;
  }
}
