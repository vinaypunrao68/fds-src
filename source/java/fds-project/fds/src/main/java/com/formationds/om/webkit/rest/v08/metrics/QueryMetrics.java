/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.metrics;

import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.helper.QueryHelper;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
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

import javax.servlet.http.HttpServletRequest;

/**
 * @author ptinius
 */
public class QueryMetrics
  implements RequestHandler {
  private static final Logger logger =
    LoggerFactory.getLogger( QueryMetrics.class );

  private static final Type TYPE = new TypeToken<MetricQueryCriteria>() { }.getType();
  private final AuthenticationToken token;
  private final Authorizer authorizer;

  public QueryMetrics( final Authorizer authorizer, AuthenticationToken token ) {
    super();

    this.token = token;
    this.authorizer = authorizer;
  }

  @Override
  public Resource handle( Request request, Map<String, String> routeParameters )
    throws Exception {

    HttpServletRequest requestLoggingProxy = RequestLog.newRequestLogger( request );
    try( final Reader reader = new InputStreamReader( requestLoggingProxy.getInputStream(), "UTF-8" ) ) {

      final Statistics stats = new QueryHelper().execute(
        ObjectModelHelper.toObject( reader, TYPE ), authorizer, token );

      return new JsonResource( new JSONObject( stats ) );
    }
  }
}
