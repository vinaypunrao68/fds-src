package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class GetContainer implements RequestHandler {
    private Xdi xdi;

    public GetContainer(Xdi xdi) {
        this.xdi = xdi;
    }

    enum OutputFormat {
        plain,
        json,
        xml
    }
    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");

        int limit = optionalInt(request, "limit", Integer.MAX_VALUE);
        OutputFormat format = obtainFormat(request);
        return null;
    }

    private OutputFormat obtainFormat(Request request) {
        return null;
    }
}
