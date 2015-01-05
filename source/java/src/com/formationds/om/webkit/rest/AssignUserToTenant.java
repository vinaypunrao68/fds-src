package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class AssignUserToTenant implements RequestHandler {
    private ConfigurationApi configCache;
    private SecretKey secretKey;

    public AssignUserToTenant(ConfigurationApi configCache, SecretKey secretKey) {
        this.configCache = configCache;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        long tenantId = requiredLong(routeParameters, "tenantid");
        long userId = requiredLong(routeParameters, "userid");
        configCache.assignUserToTenant(userId, tenantId);
        return new JsonResource(new JSONObject().put("status", "ok"));
    }
}
