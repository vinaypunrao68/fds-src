package com.formationds.om.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.HashedPassword;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationServiceCache;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;
import java.util.UUID;

public class CreateUser implements RequestHandler {
    private ConfigurationServiceCache configCache;
    private SecretKey secretKey;

    public CreateUser(ConfigurationServiceCache configCache, SecretKey secretKey) {
        this.configCache = configCache;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String login = requiredString(routeParameters, "login");
        String password = requiredString(routeParameters, "password");

        String hashed = new HashedPassword().hash(password);
        String secret = UUID.randomUUID().toString();
        long id = configCache.createUser(login, hashed, secret, false);
        return new JsonResource(new JSONObject().put("id", Long.toString(id)));
    }
}
