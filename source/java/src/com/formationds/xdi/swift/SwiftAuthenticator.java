package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.function.Supplier;

public class SwiftAuthenticator implements RequestHandler {
    private Supplier<RequestHandler> supplier;

    public SwiftAuthenticator(Supplier<RequestHandler> supplier) {
        this.supplier = supplier;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String tokenHeader = request.getHeader(Authenticate.X_AUTH_TOKEN);
        if (tokenHeader == null) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }

        AuthenticationToken token = new AuthenticationToken(Authenticator.KEY, tokenHeader);
        if (token.isValid()) {
            return supplier.get().handle(request, routeParameters);
        } else {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
    }
}
