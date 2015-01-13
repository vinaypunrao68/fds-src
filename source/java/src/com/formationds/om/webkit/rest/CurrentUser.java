/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest;

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CurrentUser implements RequestHandler {
    private Authorizer  authz;
    private AuthenticationToken t;

    public CurrentUser(Authorizer authz, AuthenticationToken t) {
        this.authz = authz;
        this.t = t;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        User user = authz.userFor(t);
        // {id:42, identifier:"paul", isFdsAdmin:false}

        JSONObject o = new JSONObject()
                           .put("id", Long.toString(user.getId()))
                           .put("identifier", user.getIdentifier())
                           .put("isFdsAdmin", user.isIsFdsAdmin());

        return new JsonResource(o);
    }
}
