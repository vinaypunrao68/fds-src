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

public class DeleteObject implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        if (ctx.getQueryParameters().containsKey("uploadId"))
            return new MultiPartUploadAbort(xdi, token).handle(ctx);

        String bucketName = ctx.getRouteParameter("bucket");
        String objectName = ctx.getRouteParameter("object");
        xdi.deleteBlob(token, S3Endpoint.FDS_S3, bucketName, S3Namespace.user().blobName(objectName)).get();
        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
