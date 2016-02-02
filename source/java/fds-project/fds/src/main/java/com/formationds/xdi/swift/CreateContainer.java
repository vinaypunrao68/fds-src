package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CreateContainer implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public CreateContainer(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");

        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, Long.MAX_VALUE, 0, MediaPolicy.HDD_ONLY);
        xdi.createVolume(token, accountName, containerName, volumeSettings);
        return SwiftUtility.swiftResource(new TextResource(201, ""));
    }
}
