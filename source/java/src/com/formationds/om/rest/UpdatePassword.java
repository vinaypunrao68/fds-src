package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.security.HashedPassword;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationServiceCache;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;
import java.util.Optional;

public class UpdatePassword implements RequestHandler {
    private ConfigurationServiceCache configCache;
    private SecretKey secretKey;

    public UpdatePassword(ConfigurationServiceCache configCache, SecretKey secretKey) {
        this.configCache = configCache;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long userId = requiredLong(routeParameters, "userid");
        String password = requiredString(routeParameters, "password");
        String hashedPassword = new HashedPassword().hash(password);

        Optional<User> opt = configCache.allUsers(0).stream()
                .filter(u -> userId == u.getId())
                .findFirst();

        if (!opt.isPresent()) {
            return new JsonResource(new JSONObject().put("status", "No such user"), 404);
        }

        User u = opt.get();
        configCache.updateUser(u.id, u.getIdentifier(), hashedPassword, u.getSecret(), u.isIsFdsAdmin());
        return new JsonResource(new JSONObject().put("status", "OK"));
    }
}
