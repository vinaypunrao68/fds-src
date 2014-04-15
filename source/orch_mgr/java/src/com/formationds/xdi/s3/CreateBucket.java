package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.AmShim;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.local.LocalAmShim;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class CreateBucket implements RequestHandler {
    private Xdi xdi;

    public CreateBucket(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        xdi.createVolume(bucketName, 1024 * 1024 * 4);
        return new TextResource("");
    }
}
