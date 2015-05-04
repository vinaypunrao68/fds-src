package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.VolumeStatus;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class DeleteContainer implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteContainer(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");
        VolumeStatus status = xdi.statVolume(token, accountName, containerName).get();

        if (status.getBlobCount() != 0) {
            TextResource tr = new TextResource(409, "There was a conflict when trying to complete your request.");
            return SwiftUtility.swiftResource(tr);
        } else {
            xdi.deleteVolume(token, accountName, containerName);
            TextResource tr = new TextResource(204, "");
            return SwiftUtility.swiftResource(tr);
        }
    }
}
