package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

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

public class ListUsers implements RequestHandler {
    private final ConfigurationApi cache;
    private final SecretKey secretKey;
    private final CachedConfiguration cachedConfig;

    public ListUsers(ConfigurationApi cache, SecretKey secretKey) {
        this.cache = cache;
        this.secretKey = secretKey;
        cachedConfig = cache.get();
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray();
        cache.allUsers(0).stream()
                .map(u -> {
                    JSONObject o = new JSONObject();
                    o.put("id", u.getId())
                            .put("identifier", u.getIdentifier())
                            .put("isFdsAdmin", u.isIsFdsAdmin());

                    cachedConfig.tenantFor(u.getId())
                            .ifPresent(t -> o.put("tenant",
                                    new JSONObject()
                                            .put("id", Long.toString(t.getId()))
                                            .put("name", t.getIdentifier())));
                    return o;
                })
                .forEach(o -> array.put(o));

        return new JsonResource(array);
    }
}
