package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.am.Main;
import com.formationds.apis.BlobDescriptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StreamResource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.collect.LinkedListMultimap;
import com.google.common.collect.Multimap;
import org.apache.commons.codec.binary.Hex;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.io.InputStream;
import java.nio.ByteBuffer;
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
        return handle(request, bucketName, objectName);
    }

    public Resource handle(Request request, String bucketName, String objectName) throws Exception {
        BlobDescriptor blobDescriptor = xdi.statBlob(Main.FDS_S3, bucketName, objectName);

        String headerValue = request.getHeader("If-None-Match");
        if (headerValue != null) {
            ByteBuffer candidateValue = ByteBuffer.wrap(Hex.decodeHex(headerValue.toCharArray()));
            if (candidateValue.compareTo(blobDescriptor.bufferForDigest()) == 0) {
                return new TextResource(HttpServletResponse.SC_NOT_MODIFIED, "");
            }
        }

        String contentType = blobDescriptor.getMetadata().getOrDefault("Content-Type", "application/octet-stream");
        String etag = blobDescriptor.getMetadata().getOrDefault("etag", "Yo");
        InputStream stream = xdi.readStream(Main.FDS_S3, bucketName, objectName);
        return new StreamResource(stream, contentType) {
            @Override
            public Multimap<String, String> extraHeaders() {
                LinkedListMultimap<String, String> map = LinkedListMultimap.create();
                // TODO(Andrew): I'm pulling the etag from the k/v metadata where
                // it's stored as a hex string already...can probably remove digest
                // map.put("Etag", Hex.encodeHexString(blobDescriptor.getDigest()));
                map.put("Etag", "\"" + etag + "\"");
                return map;
            }
        };
    }
}
