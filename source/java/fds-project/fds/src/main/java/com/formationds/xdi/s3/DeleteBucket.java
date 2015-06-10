package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;

import javax.servlet.http.HttpServletResponse;

public class DeleteBucket implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteBucket(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
        String bucketName = context.getRouteParameter("bucket");
        xdi.deleteVolume(token, S3Endpoint.FDS_S3, bucketName);
        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
