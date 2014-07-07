package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthorizationToken;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.sun.security.auth.UserPrincipal;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class IssueToken implements RequestHandler {
    private SecretKey secretKey;

    public IssueToken(SecretKey secretKey) {
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(request, "login");
        String password = requiredString(request, "password");

        if ("admin".equals(login) && "admin".equals(password)) {
            AuthorizationToken token = new AuthorizationToken(secretKey, new UserPrincipal("admin"));
            return new JsonResource(new JSONObject().put("token", token.getKey().toBase64())) {
                @Override
                public Cookie[] cookies() {
                    Cookie cookie = new Cookie(Authorizer.FDS_TOKEN, token.toString());
                    cookie.setPath("/");
                    return new Cookie[]{cookie};
                }
            };
        } else {
            JSONObject message = new JSONObject().put("message", "Invalid credentials.");
            return new JsonResource(message, HttpServletResponse.SC_UNAUTHORIZED);
        }
    }
}
