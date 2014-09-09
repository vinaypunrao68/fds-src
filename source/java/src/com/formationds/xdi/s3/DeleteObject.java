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

public class DeleteObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        if(request.getQueryParameters().containsKey("uploadId"))
            return new MultiPartUploadAbort(xdi, token).handle(request, routeParameters);

        String bucketName = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        xdi.deleteBlob(token, S3Endpoint.FDS_S3, bucketName, objectName);
        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
