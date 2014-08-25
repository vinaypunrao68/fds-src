package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.function.Supplier;

public class SwiftAuthenticator implements RequestHandler {
    private Supplier<RequestHandler> supplier;
    private Xdi xdi;

    public SwiftAuthenticator(Supplier<RequestHandler> supplier, Xdi xdi) {
        this.supplier = supplier;
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String tokenHeader = request.getHeader(GrantAuthenticationToken.X_AUTH_TOKEN);
        if (tokenHeader == null) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }

        try {
            xdi.resolveToken(tokenHeader);
            return supplier.get().handle(request, routeParameters);
        } catch (LoginException exception) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
    }
}
