package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletResponse;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class GrantToken implements RequestHandler {
    private Xdi xdi;
    private SecretKey key;

    public GrantToken(Xdi xdi, SecretKey key) {
        this.xdi = xdi;
        this.key = key;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(request, "login");
        String password = requiredString(request, "password");

        final List<String> features = new ArrayList<>();
        final JSONObject jsonObject = new JSONObject();

        try {
            AuthenticationToken token = xdi.getAuthenticator()
                                           .authenticate(login, password);

            if (login.equalsIgnoreCase("admin")) {
                features.add("System Management");
                features.add("Volume Management");
                features.add("Tenant Management");
                features.add("User Management");
            } else {
                features.add("Volume Management");
                features.add("User Management");
            }

            final User user = xdi.getAuthorizer().userFor( token );
            // temporary work-a-round for goldman
            jsonObject.put("username", login);
            jsonObject.put("userId", user.getId() );
            jsonObject.put("token", token.signature(key));
            jsonObject.put("features", features);
            // end of work-a-round

            // new JSONObject().put("token", token.signature(Authenticator.KEY))
            return new JsonResource(jsonObject) {
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
