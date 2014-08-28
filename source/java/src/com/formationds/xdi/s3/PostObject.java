package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class PostObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public PostObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        if (request.getParameter("delete") != null) {
            return new DeleteMultipleObjects(xdi, token, bucketName).handle(request, routeParameters);
        } else if(request.getParameter("uploads") != null) {
            return new MultiPartUploadInitiate(xdi, token).handle(request, routeParameters);
        } else if(request.getParameter("uploadId") != null) {
            return new MultiPartUploadComplete(xdi, token).handle(request, routeParameters);
        } else {
            return new PostObjectUpload(xdi, token).handle(request, routeParameters);
        }

    }
}
