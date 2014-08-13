package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class PutObject implements RequestHandler {
    private Xdi xdi;

    public PutObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        if (request.getParameter("uploadId") != null) {
            return new MultiPartUploadPutPart(xdi).handle(request, routeParameters);
        } else {
            return new PutObjectUpload(xdi).handle(request, routeParameters);
        }
    }
}
