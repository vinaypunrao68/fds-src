/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest.v08.users;

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

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
    	return new TextResource( "ok" );
    }
}
