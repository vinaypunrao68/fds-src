/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.rest.metrics;

import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.Map;

/**
 * @author ptinius
 */
public class QueryMetrics
  implements RequestHandler {
  private static final transient Logger logger =
    LoggerFactory.getLogger( QueryMetrics.class );

  private static final Type TYPE = new TypeToken<QueryCriteria>() { }.getType();

  public QueryMetrics() {
    super();
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {

    try( final Reader reader =
           new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {

      final Statistics stats = new QueryHelper().execute(
        ObjectModelHelper.toObject( reader, TYPE ) );

      logger.trace( "STATS: {} ", stats );

      return new JsonResource( new JSONObject( stats ) );
    }
  }
}
