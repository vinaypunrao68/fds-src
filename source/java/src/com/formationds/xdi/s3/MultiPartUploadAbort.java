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
        String uploadId = context.getQueryString().get("uploadId").getFirst();
        MultiPartOperations mops = new MultiPartOperations(xdi, uploadId, token);

        for(PartInfo pi : mops.getParts()) {
            String systemVolume = xdi.getSystemVolumeName(token);
            xdi.deleteBlob(token, S3Endpoint.FDS_S3_SYSTEM, systemVolume, pi.descriptor.getName());
        }

        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
