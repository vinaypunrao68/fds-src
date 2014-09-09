package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.util.Map;

public class MultiPartUploadAbort implements RequestHandler {

    private Xdi xdi;
    private AuthenticationToken token;

    public MultiPartUploadAbort(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        String uploadId = request.getQueryParameters().getString("uploadId");
        MultiPartOperations mops = new MultiPartOperations(xdi, uploadId, token);

        for(PartInfo pi : mops.getParts()) {
            xdi.deleteBlob(token, S3Endpoint.FDS_S3_SYSTEM, S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME, pi.descriptor.getName());
        }

        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }


}
