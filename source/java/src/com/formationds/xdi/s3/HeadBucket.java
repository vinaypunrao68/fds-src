package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.am.Main;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.shim.VolumeDescriptor;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class HeadBucket implements RequestHandler {
    private Xdi xdi;

    public HeadBucket(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        VolumeDescriptor descriptor = xdi.statVolume(Main.FDS_S3, bucketName);
        return new TextResource("");
    }
}
