package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.AuthenticatedXdi;

public class CreateBucket implements SyncRequestHandler {
    private AuthenticatedXdi xdi;
    private AuthenticationToken token;

    public CreateBucket(AuthenticatedXdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
        String bucketName = context.getRouteParameter("bucket");
        VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, Long.MAX_VALUE, 24 * 60 * 60 /** one day **/, MediaPolicy.HDD_ONLY);
        xdi.createVolume(token, S3Endpoint.FDS_S3, bucketName, volumeSettings);
        return new TextResource("");
    }
}
