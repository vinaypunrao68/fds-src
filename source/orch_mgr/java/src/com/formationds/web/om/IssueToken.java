package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.auth.AuthenticationToken;
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
        String login = requiredString(routeParameters, "login");
        String password = requiredString(routeParameters, "password");

        if ("admin".equals(login) && "admin".equals(password)) {
            AuthenticationToken token = new AuthenticationToken(secretKey, new UserPrincipal("admin"));
            return new JsonResource(new JSONObject().put("token", token.toString())) {
                @Override
                public Cookie[] cookies() {
                    Cookie cookie = new Cookie(Authorizer.FDS_TOKEN, token.toString());
                    cookie.setPath("/");
                    return new Cookie[] {cookie};
                }
            };
        } else {
            JSONObject message = new JSONObject().put("message", "Invalid credentials.");
            return new JsonResource(message, HttpServletResponse.SC_UNAUTHORIZED);
        }
    }

}
