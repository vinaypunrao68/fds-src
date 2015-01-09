package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import java.util.Map;

public class CurrentUser implements RequestHandler {
    private ConfigurationApi config;
    private AuthenticationToken t;

    public CurrentUser(ConfigurationApi config, AuthenticationToken t) {
        this.config = config;
        this.t = t;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        User user = config.userFor(t);
        // {id:42, identifier:"paul", isFdsAdmin:false}

        JSONObject o = new JSONObject()
                .put("id", Long.toString(user.getId()))
                .put("identifier", user.getIdentifier())
                .put("isFdsAdmin", user.isIsFdsAdmin());

        return new JsonResource(o);
    }
}
