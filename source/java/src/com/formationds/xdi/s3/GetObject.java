package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.am.Main;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StreamResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.shim.BlobDescriptor;
import org.eclipse.jetty.server.Request;

import java.io.InputStream;
import java.util.Map;

public class GetObject implements RequestHandler {
    private Xdi xdi;

    public GetObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
        BlobDescriptor blobDescriptor = xdi.statBlob(Main.FDS_S3, bucketName, objectName);
        String contentType = blobDescriptor.getMetadata().getOrDefault("Content-Type", "application/octet-stream");
        InputStream stream = xdi.readStream(Main.FDS_S3, bucketName, objectName);
        return new StreamResource(stream, contentType);
    }
}
