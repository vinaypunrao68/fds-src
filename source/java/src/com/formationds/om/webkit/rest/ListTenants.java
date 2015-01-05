package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.Tenant;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.CachedConfiguration;
import com.formationds.xdi.ConfigurationApi;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class ListTenants implements RequestHandler {
    private ConfigurationApi configCache;
    private CachedConfiguration config;
    private SecretKey secretKey;

    public ListTenants(ConfigurationApi configCache, SecretKey secretKey) {
        this.configCache = configCache;
        this.secretKey = secretKey;
        this.config = configCache.get();
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray();
        configCache.listTenants(0).stream()
                .map(t -> {
                    JSONObject o = new JSONObject();
                    o.put("id", Long.toString(t.getId()));
                    o.put("name", t.getIdentifier());
                    o.put("users", users(t));
                    return o;
                })
                .forEach(array::put);

        return new JsonResource(array);
    }

    private JSONArray users(Tenant t) {
        JSONArray array = new JSONArray();
        config.listUsersForTenant(t.getId()).stream()
                .map(u -> new JSONObject()
                        .put("id", Long.toString(u.getId()))
                        .put("identifier", u.getIdentifier()))
                .forEach(array::put);
        return array;
    }
}
