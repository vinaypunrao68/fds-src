/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest.events;

import com.formationds.commons.model.entity.EventQuery;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.GsonBuilder;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.io.InputStreamReader;
import java.io.Reader;
import java.util.Map;

/**
 * @author dsetzke based on QueryMetrics by ptinius
 */
public class QueryEvents
  implements RequestHandler {

  public QueryEvents() {
    super();
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {

    try( final Reader reader =
           new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {
      final EventQuery eventQuery = new GsonBuilder().create()
                                                       .fromJson( reader,
                                                                  EventQuery.class );


      // TODO finish implementation
    }

    return new JsonResource( new JSONObject().put( "status", "OK" ) );
  }
}
