package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.ApiException;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class HttpErrorHandler implements RequestHandler {
    private static final Logger logger =
        LoggerFactory.getLogger( HttpErrorHandler.class );

    private RequestHandler rh;

    public HttpErrorHandler(RequestHandler rh) {
        this.rh = rh;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        try {

            return rh.handle(request, routeParameters);

        } catch (Exception e) {

            logger.error( "Error executing {} -- {}", request.getRequestURI(), e.getMessage() );
            logger.trace( "Stack Trace::", e );

            JSONObject response = new JSONObject();
            response.put( "type", e.getClass().getName() );

            if( e.getMessage() != null && e.getMessage().length() > 0 ) {
                response.put( "message", e.getMessage() );
            } else {
                response.put( "message", "Internal server error" );
            }

            if (e instanceof ApiException) {
                response.put( "error_code", ((ApiException)e).getErrorCode() );
            }

            return new JsonResource( response, HttpServletResponse.SC_INTERNAL_SERVER_ERROR );
        }
    }
}
