package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.swift.SwiftUtility;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;

import java.util.Map;

public class HeadObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public HeadObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String volume = requiredString(routeParameters, "bucket");
        String object = requiredString(routeParameters, "object");

        BlobDescriptor stat = xdi.statBlob(token, S3Endpoint.FDS_S3, volume, object);
        Map<String, String> metadata = stat.getMetadata();
        String contentType = metadata.getOrDefault("Content-Type", StaticFileHandler.getMimeType(object));
        String lastModified = metadata.getOrDefault("Last-Modified", SwiftUtility.formatRfc1123Date(DateTime.now()));
        long byteCount = stat.getByteCount();
        return new TextResource("")
                .withHeader("Content-Length", Long.toString(byteCount))
                .withHeader("Content-Type", contentType)
                .withHeader("Last-Modified", lastModified);
    }
}
