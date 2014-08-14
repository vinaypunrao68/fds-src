package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class MultiPartUploadAbort implements RequestHandler {

    private Xdi xdi;

    public MultiPartUploadAbort(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        String uploadId = request.getParameter("uploadId");
        MultiPartOperations mops = new MultiPartOperations(xdi, uploadId);

        for(PartInfo pi : mops.getParts()) {
            xdi.deleteBlob(S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, pi.descriptor.getName());
        }

        return new TextResource("");
    }


}
