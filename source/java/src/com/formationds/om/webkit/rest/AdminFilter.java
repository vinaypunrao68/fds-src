package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class AdminFilter implements RequestHandler {
    private AuthenticationToken t;
    private Authorizer authorizer;
    private RequestHandler rh;

    public AdminFilter(AuthenticationToken t, Authorizer authorizer, RequestHandler rh) {
        this.t = t;
        this.authorizer = authorizer;
        this.rh = rh;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        try {
            if (authorizer.userFor(t).isFdsAdmin) {
                return rh.handle(request, routeParameters);
            } else {
                return new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
            }
        } catch (Exception e) {
            return new JsonResource(new JSONObject().put("message", "Invalid permissions"), HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
