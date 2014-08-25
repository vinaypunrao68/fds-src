package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.security.auth.login.LoginException;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class IssueToken implements RequestHandler {
    private Xdi xdi;

    public IssueToken(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(request, "login");
        String password = requiredString(request, "password");

        try {
            AuthenticationToken token = xdi.authenticate(login, password);
            return new JsonResource(new JSONObject().put("token", token.signature(Authenticator.KEY))) {
                @Override
                public Cookie[] cookies() {
                    Cookie cookie = new Cookie(HttpAuthenticator.FDS_TOKEN, token.toString());
                    cookie.setPath("/");
                    return new Cookie[]{cookie};
                }
            };
        } catch (LoginException e) {
            JSONObject message = new JSONObject().put("message", "Invalid credentials.");
            return new JsonResource(message, HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
