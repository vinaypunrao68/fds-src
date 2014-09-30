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

public class DeleteBucket implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public DeleteBucket(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        xdi.deleteVolume(token, S3Endpoint.FDS_S3, bucketName);
        return new TextResource(HttpServletResponse.SC_NO_CONTENT, "");
    }
}
