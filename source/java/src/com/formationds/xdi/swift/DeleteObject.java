package com.formationds.xdi.swift;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class DeleteObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = requiredString(routeParameters, "account");
        String volume = requiredString(routeParameters, "container");
        String object = requiredString(routeParameters, "object");

        // TODO: multipart-manifest delete behavior
        xdi.deleteBlob(token, domain, volume, object).get();

        return SwiftUtility.swiftResource(new TextResource(200, ""));
    }
}
