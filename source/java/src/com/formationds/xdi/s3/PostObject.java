package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class PostObject implements RequestHandler {
    private Xdi xdi;

    public PostObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        if (request.getParameter("delete") != null) {
            return new DeleteMultipleObjects(xdi, bucketName).handle(request, routeParameters);
        } else if(request.getParameter("uploads") != null) {
            return new MultiPartUploadInitiate(xdi).handle(request, routeParameters);
        } else if(request.getParameter("uploadId") != null) {
            return new MultiPartUploadComplete(xdi).handle(request, routeParameters);
        } else {
            return new PostObjectUpload(xdi).handle(request, routeParameters);
        }

    }
}
