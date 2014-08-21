package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.security.LoginModule;
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
        String tokenHeader = request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN);
        if (tokenHeader == null) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }

        if (AuthenticationToken.isValid(LoginModule.KEY, tokenHeader)) {
            return supplier.get().handle(request, routeParameters);
        } else {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
    }
}
