package com.formationds.om.webkit.rest.v08.users;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

import org.eclipse.jetty.server.Request;
import javax.crypto.SecretKey;

import java.util.Map;

public class ListUsers implements RequestHandler {
    private final ConfigurationApi cache;
    private final SecretKey        secretKey;

    public ListUsers(ConfigurationApi cache, SecretKey secretKey) {
        this.cache = cache;
        this.secretKey = secretKey;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {


        return new TextResource( "ok" );
    }
}
