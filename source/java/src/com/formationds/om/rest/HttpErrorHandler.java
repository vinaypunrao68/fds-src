package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class HttpErrorHandler implements RequestHandler {
    private static final Logger LOG = Logger.getLogger(HttpErrorHandler.class);

    private RequestHandler rh;

    public HttpErrorHandler(RequestHandler rh) {
        this.rh = rh;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        try {
            return rh.handle(request, routeParameters);
        } catch (Exception e) {
            LOG.error("Error executing " + request.getRequestURI(), e);
            if( e.getMessage() != null && e.getMessage().length() > 1 ) {
                return new JsonResource(
                    new JSONObject().put( "message",
                                          e.getMessage() ),
                    HttpServletResponse.SC_INTERNAL_SERVER_ERROR );
            }

            return new JsonResource(
                new JSONObject().put( "message",
                                      "Internal server error" ),
                HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }
}
