/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.tenants;

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;

import javax.crypto.SecretKey;

import java.util.Map;

public class ListTenants implements RequestHandler {
    private ConfigurationApi config;
    private SecretKey        secretKey;

    public ListTenants(ConfigurationApi configCache, SecretKey secretKey) {
        this.secretKey = secretKey;
        this.config = configCache;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
       return new TextResource( "ok" );
    }
}
