/**
 * Copyright (c) 2015 Formation Data Systems. All rights reserved.
 */
package com.formationds.om.webkit.rest.v07.users;

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.security.HashedPassword;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import javax.servlet.http.HttpServletResponse;
import java.util.Map;
import java.util.Optional;

public class UpdatePassword implements RequestHandler {
    private AuthenticationToken token;
    private ConfigurationApi configCache;
    private SecretKey                                    secretKey;
    private Authorizer                                   authorizer;

    public UpdatePassword(AuthenticationToken token, ConfigurationApi configCache,
                          SecretKey secretKey, Authorizer authorizer) {
        this.token = token;
        this.configCache = configCache;
        this.secretKey = secretKey;
        this.authorizer = authorizer;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long targetUserId = requiredLong(routeParameters, "userid");
        String password = requiredString(routeParameters, "password");
        User currentUser = authorizer.userFor(token);

        return execute(currentUser, targetUserId, password);
    }

    public Resource execute(User currentUser, long userId, String password) throws TException {
        if (!currentUser.isIsFdsAdmin() && userId != currentUser.getId()) {
            return new JsonResource(new JSONObject().put("status", "Access denied"), HttpServletResponse.SC_UNAUTHORIZED);
        }

        String hashedPassword = new HashedPassword().hash(password);

        Optional<User> opt = configCache.allUsers(0).stream()
                .filter(u -> userId == u.getId())
                .findFirst();

        if (!opt.isPresent()) {
            return new JsonResource(new JSONObject().put("status", "No such user"), 404);
        }

        User u = opt.get();
        configCache.updateUser(u.getId(), u.getIdentifier(), hashedPassword, u.getSecret(), u.isIsFdsAdmin());
        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
