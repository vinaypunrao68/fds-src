package com.formationds.xdi.swift;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.util.Map;

public class DeleteObject implements RequestHandler {
    private Xdi xdi;

    public DeleteObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String domain = requiredString(routeParameters, "account");
        String volume = requiredString(routeParameters, "container");
        String object = requiredString(routeParameters, "object");

        // TODO: multipart-manifest delete behavior
        xdi.deleteBlob(domain, volume, object);

        return SwiftUtility.swiftResource(new TextResource(200, ""));
    }
}
