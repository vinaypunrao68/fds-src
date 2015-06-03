/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08;

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

            if( e.getMessage() != null && e.getMessage().length() > 1 ) {

                logger.error( "Error executing " +
                              request.getRequestURI() +
                              " -- " +
                              e.getMessage() );
                logger.trace( "Stack Trace::", e );

                return new JsonResource(
                    new JSONObject().put( "message",
                                          e.getMessage() ),
                    HttpServletResponse.SC_INTERNAL_SERVER_ERROR );
            }

            logger.error( "Error executing " +
                          request.getRequestURI() );
            logger.trace( "Stack Trace::", e );

            return new JsonResource(
                new JSONObject().put( "message",
                                      "Internal server error" ),
                HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }
}
