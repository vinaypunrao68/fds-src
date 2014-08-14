package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.BlobDescriptor;
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

import javax.servlet.http.HttpServletResponse;
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
        String domain  = S3Endpoint.FDS_S3;
        String bucketName = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");

        String copySource = request.getHeader("x-amz-copy-source");

        String contentType = StaticFileHandler.getMimeType(objectName);
        String uploadId = request.getParameter("uploadId");

        HashMap<String, String> map = Maps.newHashMap();
        map.put("Content-Type", contentType);

        // handle multi part upload
        if(uploadId != null) {
            MultiPartOperations mops = new MultiPartOperations(xdi, uploadId);
            int partNumber = Integer.parseInt(request.getParameter("partNumber"));

            if(partNumber < 0 || partNumber > 10000)
                throw new Exception("invalid part number");

            domain = S3Endpoint.FDS_S3_SYSTEM;
            bucketName = S3Endpoint.FDS_S3_SYSTEM_BUCKET_NAME;
            objectName = mops.getPartName(partNumber);
        }

        if(copySource == null) {
            byte[] digest = xdi.writeStream(domain, bucketName, objectName, request.getInputStream(), map);
            return new TextResource("").withHeader("ETag", "\"" + Hex.encodeHexString(digest) + "\"");
        } else {
            return copy(request, domain, bucketName, objectName, copySource, map);
        }
    }

    private Resource copy(Request request, String targetDomain, String targetBucketName, String targetBlobName, String copySource, HashMap<String, String> metadataMap) throws Exception {
        String[] copySourceParts = copySource.split("/", 2);
        if(copySourceParts.length != 2)
            throw new Exception("invalid copy source");

        String copySourceBucket = copySourceParts[0];
        String copySourceObject = copySourceParts[1];
        BlobDescriptor copySourceStat = xdi.statBlob(S3Endpoint.FDS_S3, copySourceBucket, copySourceObject);

        String metadataDirective = request.getHeader("x-amz-metadata-directive");
        if(metadataDirective != null && metadataDirective.equals("COPY")) {
            metadataMap = new HashMap<>(copySourceStat.getMetadata());

        } else if(metadataDirective != null && !metadataDirective.equals("REPLACE")) {
            return new TextResource(HttpServletResponse.SC_BAD_REQUEST, "");
        }

        String copySourceETag = normalizeETag(copySourceStat.getMetadata().getOrDefault("etag", null));
        String matchETag = normalizeETag(request.getHeader("x-amz-copy-source-if-match"));
        String notMatchETag = normalizeETag(request.getHeader("x-amz-copy-source-if-match"));
        if(matchETag != null && !matchETag.equals(copySourceETag))
            return new TextResource(HttpServletResponse.SC_PRECONDITION_FAILED, "");
        if(notMatchETag != null && notMatchETag.equals(copySourceETag))
            return new TextResource(HttpServletResponse.SC_PRECONDITION_FAILED, "");

        // TODO: last modified checks - x-amz-copy-source-if-unmodified-since and x-amz-copy-source-if-modified-since

        InputStream instr = xdi.readStream(S3Endpoint.FDS_S3, copySourceParts[0], copySourceParts[1]);
        byte[] digest = xdi.writeStream(targetDomain, targetBucketName, targetBlobName, instr, metadataMap);
        return new TextResource("").withHeader("ETag", "\"" + Hex.encodeHexString(digest) + "\"");
    }

    private String normalizeETag(String input) {
        if(input != null)
            return input.trim().toLowerCase().replaceAll("[^0-9a-f]", "");

        return null;
    }
}
