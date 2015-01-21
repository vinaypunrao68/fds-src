package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.function.Function;

public class SwiftAuthenticator implements RequestHandler {
    private Function<AuthenticationToken, RequestHandler> f;
    private Authenticator authenticator;

    public SwiftAuthenticator(Function<AuthenticationToken, RequestHandler> f, Authenticator authenticator) {
        this.f = f;
        this.authenticator = authenticator;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        if (authenticator.allowAll()) {
            return f.apply(AuthenticationToken.ANONYMOUS).handle(request, routeParameters);
        }

        String tokenHeader = request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN);
        if (tokenHeader == null) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }

        try {
            AuthenticationToken token = authenticator.resolveToken(tokenHeader);
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
