package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CurrentUser implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken t;

    public CurrentUser(Xdi xdi, AuthenticationToken t) {
        this.xdi = xdi;
        this.t = t;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        User user = xdi.getAuthorizer().userFor(t);
        // {id:42, identifier:"paul", isFdsAdmin:false}

        JSONObject o = new JSONObject()
                .put("id", Long.toString(user.getId()))
                .put("identifier", user.getIdentifier())
                .put("isFdsAdmin", user.isIsFdsAdmin());

        return new JsonResource(o);
    }
}
