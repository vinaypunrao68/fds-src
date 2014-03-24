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
import javax.crypto.spec.SecretKeySpec;
import javax.servlet.http.HttpServletResponse;

public class AuthenticateAction implements RequestHandler {
    private static final SecretKey KEY = new SecretKeySpec(new byte[]{35, -37, -53, -105, 107, -37, -14, -64, 28, -74, -98, 124, -8, -7, 68, 54}, "AES");

    @Override
    public Resource handle(Request request) throws Exception {
        String login = assertParameter(request, "login");
        String password = assertParameter(request, "password");

        if ("admin".equals(login) && "admin".equals(password)) {
            AuthenticationToken token = new AuthenticationToken(KEY, new UserPrincipal("admin"));
            return new JsonResource(new JSONObject().put("token", token.toString()));
        } else {
            JSONObject message = new JSONObject().put("message", "Invalid credentials.");
            return new JsonResource(message, HttpServletResponse.SC_UNAUTHORIZED);
        }
    }

}
