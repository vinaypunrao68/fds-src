/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class AssignUserToTenant implements RequestHandler {
    private com.formationds.util.thrift.ConfigurationApi configCache;
    private SecretKey                                    secretKey;

    public AssignUserToTenant(com.formationds.util.thrift.ConfigurationApi configCache, SecretKey secretKey) {
        this.configCache = configCache;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

        return new JsonResource(new JSONObject().put("status", "ok"));
    }
}
