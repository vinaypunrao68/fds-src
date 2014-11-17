package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CreateBucket implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public CreateBucket(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        long tenantId = xdi.getAuthorizer().tenantId(token);
        xdi.createVolume(token, S3Endpoint.FDS_S3, bucketName, new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, tenantId, 0));
        return new TextResource("");
    }
}
