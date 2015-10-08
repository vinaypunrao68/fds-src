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
import com.formationds.xdi.io.BlobSpecifier;

import javax.servlet.http.HttpServletResponse;

public class MultiPartUploadAbort implements SyncRequestHandler {

    private Xdi xdi;
    private AuthenticationToken token;

    public MultiPartUploadAbort(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
        String bucket = context.getRouteParameter("bucket");
        String objectName = context.getRouteParameter("object");
        String uploadId = context.getQueryParameters().get("uploadId").iterator().next();
        BlobSpecifier specifier = new BlobSpecifier(S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(objectName));

        xdi.multipart(token, specifier, uploadId).cancel().get();

        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
