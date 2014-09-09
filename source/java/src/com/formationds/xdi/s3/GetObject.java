package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StreamResource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletResponse;
import java.io.InputStream;
import java.util.Map;

public class GetObject implements RequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public GetObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        if(request.getQueryParameters().containsKey("uploadId"))
            return new MultiPartListParts(xdi, token).handle(request, routeParameters);

        String bucketName = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");

        return handle(request, bucketName, objectName);
    }

    public Resource handle(Request request, String bucketName, String objectName) throws Exception {
        BlobDescriptor blobDescriptor = xdi.statBlob(token, S3Endpoint.FDS_S3, bucketName, objectName);
        String digest = blobDescriptor.getMetadata().getOrDefault("etag", "");

        String etag = "\"" + digest + "\"";

        String headerValue = request.getHeader("If-None-Match");
        if (headerValue != null) {
            headerValue = headerValue.replaceAll("\"", "");
            if (headerValue.equals(digest)) {
                return new TextResource(HttpServletResponse.SC_NOT_MODIFIED, "")
                        .withHeader("ETag", etag);
            }
        }

        String contentType = blobDescriptor.getMetadata().getOrDefault("Content-Type", "application/octet-stream");
        InputStream stream = xdi.readStream(token, S3Endpoint.FDS_S3, bucketName, objectName,
                0, blobDescriptor.getByteCount());
        return new StreamResource(stream, contentType).withHeader("ETag", etag);
    }
}
