/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.om.webkit.rest;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class CreateTenant implements RequestHandler {
    private ConfigurationApi config;
    private SecretKey                                    secretKey;

    public CreateTenant(ConfigurationApi config, SecretKey secretKey) {
        this.config = config;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String tenantName = requiredString(routeParameters, "tenant");
        long tenantId = config.createTenant(tenantName);
        return new JsonResource(new JSONObject().put("id", Long.toString(tenantId)));

        // returns {"id": "42"}
    }
}
