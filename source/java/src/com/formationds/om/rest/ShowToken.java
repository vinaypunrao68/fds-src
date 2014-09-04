package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.CachedConfiguration;
import com.formationds.xdi.ConfigurationServiceCache;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class ShowToken implements RequestHandler {
    private ConfigurationServiceCache config;
    private SecretKey secretKey;

    public ShowToken(ConfigurationServiceCache config, SecretKey secretKey) {
        this.config = config;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long userId = requiredLong(routeParameters, "userid");
        CachedConfiguration cachedConfiguration = config.get();
        Map<Long, User> users = cachedConfiguration.usersById();
        if (!users.containsKey(userId)) {
            return new JsonResource(new JSONObject().put("status", "not found"), 404);
        }

        User user = users.get(userId);
        String token = new AuthenticationToken(user.getId(), user.getSecret()).signature(secretKey);
        return new JsonResource(new JSONObject().put("token", token));

        // returns {"token": "ICwGwPbTuMYWX7O9pyq+H53+S1I0L2iGI66Ca4Pf13k="}
    }
}
