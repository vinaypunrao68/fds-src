package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.security.XdiAuthorizer;
import org.eclipse.jetty.server.Request;

import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.function.Function;

public class SwiftAuthenticator implements RequestHandler {
    private Function<AuthenticationToken, RequestHandler> f;
    private XdiAuthorizer authorizer;

    public SwiftAuthenticator(Function<AuthenticationToken, RequestHandler> f, XdiAuthorizer authorizer) {
        this.f = f;
        this.authorizer = authorizer;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        if (authorizer.allowAll()) {
            return f.apply(AuthenticationToken.ANONYMOUS).handle(request, routeParameters);
        }

        String tokenHeader = request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN);
        if (tokenHeader == null) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }

        try {
            AuthenticationToken token = authorizer.parseToken(tokenHeader);
            AuthenticatedRequestContext.begin(token);
            return f.apply(token).handle(request, routeParameters);
        } catch (LoginException exception) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
        finally {
            AuthenticatedRequestContext.complete();
        }
    }
}
