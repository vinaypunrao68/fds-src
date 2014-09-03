package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class GrantAuthenticationToken implements RequestHandler {
    public static final String X_AUTH_KEY = "X-Auth-Key";
    public static final String X_AUTH_USER = "X-Auth-User";
    public static final String X_AUTH_TOKEN = "X-Auth-Token";

    private Xdi xdi;
    private SecretKey secretKey;

    public GrantAuthenticationToken(Xdi xdi, SecretKey secretKey) {
        this.xdi = xdi;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = request.getHeader(X_AUTH_USER) == null?
                requiredString(request, "login") : request.getHeader(X_AUTH_USER);

        String password = request.getHeader(X_AUTH_KEY) == null?
                requiredString(request, "password") : request.getHeader(X_AUTH_KEY);
        try {
            AuthenticationToken credentials = xdi.getAuthenticator().authenticate(login, password);
            return new TextResource(HttpServletResponse.SC_OK, "").withHeader(X_AUTH_TOKEN, credentials.signature(secretKey));
        } catch (LoginException exception) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
    }
}
