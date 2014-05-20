package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

public class PutObject implements RequestHandler {
    private Xdi xdi;

    public PutObject(Xdi xdi) {
        this.xdi = xdi;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucketName = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");

        int blockSize = xdi.volumeConfiguration(S3Endpoint.FDS_S3, bucketName).getPolicy().getMaxObjectSizeInBytes();
        InputStream stream = new BufferedInputStream(request.getInputStream(), blockSize * 2);
        String contentType = StaticFileHandler.getMimeType(objectName);

        HashMap<String, String> map = Maps.newHashMap();
        map.put("Content-Type", contentType);

        byte[] digest = xdi.writeStream(S3Endpoint.FDS_S3, bucketName, objectName, stream, map);

        return new TextResource("") {
            @Override
            public Multimap<String, String> extraHeaders() {
                LinkedListMultimap<String, String> headers = LinkedListMultimap.create();
                headers.put("ETag", "\"" + Hex.encodeHexString(digest) + "\"");
                return headers;
            }
        };
    }
}
