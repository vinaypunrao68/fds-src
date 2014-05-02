package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.VolumePolicy;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.ResourceWrapper;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CreateContainer implements RequestHandler {
    private Xdi xdi;

    public CreateContainer(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");

        try {
            xdi.createVolume(accountName, containerName, new VolumePolicy(1024 * 1024 * 2));
            return SwiftUtility.swiftResource(new TextResource(201, ""));
        } catch(Exception e) {
            return SwiftUtility.swiftResource(new TextResource(500, e.getMessage()));
        }
    }
}
