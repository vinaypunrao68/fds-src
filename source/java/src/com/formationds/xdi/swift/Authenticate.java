package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiCredentials;
import org.eclipse.jetty.server.Request;

import javax.security.auth.login.LoginException;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class Authenticate implements RequestHandler {
    public static final String X_AUTH_KEY = "X-Auth-Key";
    public static final String X_AUTH_USER = "X-Auth-User";
    public static final String X_AUTH_TOKEN = "X-Auth-Token";

    private Xdi xdi;

    public Authenticate(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = request.getHeader(X_AUTH_USER) == null?
                requiredString(request, "login") : request.getHeader(X_AUTH_USER);

        String password = request.getHeader(X_AUTH_KEY) == null?
                requiredString(request, "password") : request.getHeader(X_AUTH_KEY);
        try {
            XdiCredentials credentials = xdi.authenticate(login, password);
            String token = credentials.getAuthenticationKey().toBase64();
            return new TextResource(HttpServletResponse.SC_OK, "").withHeader(X_AUTH_TOKEN, token);
        } catch (LoginException exception) {
            return new TextResource(HttpServletResponse.SC_UNAUTHORIZED, "");
        }
    }
}
