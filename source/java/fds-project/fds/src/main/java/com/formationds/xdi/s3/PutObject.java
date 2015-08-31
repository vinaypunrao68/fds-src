package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.BlobInfo;
import com.formationds.xdi.Xdi;
import com.google.common.collect.Maps;
import org.apache.commons.codec.binary.Hex;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;

import javax.servlet.http.HttpServletResponse;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.DigestOutputStream;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;

public class PutObject implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public PutObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
        String domain  = S3Endpoint.FDS_S3;
        String bucketName = context.getRouteParameter("bucket");
        String objectName = context.getRouteParameter("object");

        String copySource = context.getRequestHeader(S3Endpoint.X_AMZ_COPY_SOURCE);

        String contentType = S3Endpoint.S3_DEFAULT_CONTENT_TYPE;
        if (context.getRequestContentType() != null)
            contentType = context.getRequestContentType();

        InputStream str = context.getInputStream();

        String uploadId = null;
        if (context.getQueryParameters().get("uploadId") != null) {
            uploadId = context.getQueryParameters().get("uploadId").iterator().next();
        }

        HashMap<String, String> map = Maps.newHashMap();
        map.put("Content-Type", contentType);
        map.putAll(S3UserMetadataUtility.requestUserMetadata(context));

        // handle multi part upload
        if(uploadId != null) {
            MultiPartOperations mops = new MultiPartOperations(xdi, uploadId, token);
            int partNumber = Integer.parseInt(context.getQueryParameters().get("partNumber").iterator().next());

            if(partNumber < 0 || partNumber > 10000)
                throw new Exception("invalid part number");

            domain = S3Endpoint.FDS_S3_SYSTEM;
            bucketName = xdi.getSystemVolumeName(token);
            objectName = mops.getPartName(partNumber);
        }

        if(copySource == null) {
            byte[] digest = xdi.put(token, domain, bucketName, S3Namespace.user().blobName(objectName), str, map).get().digest;
            return new TextResource("").withHeader("ETag", "\"" + Hex.encodeHexString(digest) + "\"");
        } else {
            return copy(context, domain, bucketName, objectName, copySource, map);
        }
    }

    private Resource copy(HttpContext context, String targetDomain, String targetBucketName, String targetBlobName, String copySource, HashMap<String, String> metadataMap) throws Exception {
        String[] copySourceParts = copySource.replaceFirst("^/", "").split("/", 2);
        if(copySourceParts.length != 2)
            throw new Exception("invalid copy source");

        String copySourceBucket = copySourceParts[0];
        String copySourceObject = copySourceParts[1];
        BlobDescriptor copySourceStat = xdi.statBlob(token, S3Endpoint.FDS_S3, copySourceBucket,S3Namespace.user().blobName(copySourceObject)).get();

        String metadataDirective = context.getRequestHeader("x-amz-metadata-directive");
        if(metadataDirective != null && metadataDirective.equals("COPY")) {
            metadataMap = new HashMap<>(copySourceStat.getMetadata());

        } else if(metadataDirective != null && !metadataDirective.equals("REPLACE")) {
            return new TextResource(HttpServletResponse.SC_BAD_REQUEST, "");
        }

        String copySourceETag = normalizeETag(copySourceStat.getMetadata().getOrDefault("etag", null));
        String matchETag = normalizeETag(context.getRequestHeader("x-amz-copy-source-if-match"));
        String notMatchETag = normalizeETag(context.getRequestHeader("x-amz-copy-source-if-match"));

        DateTime lastModifiedTemp;
        try {
            long instant = DateTimeZone.getDefault().convertLocalToUTC(Long.parseLong(copySourceStat.getMetadata().get(Xdi.LAST_MODIFIED)), false);
            lastModifiedTemp = new DateTime(instant, DateTimeZone.UTC);
        } catch(Exception ex) {
            lastModifiedTemp = DateTime.now(DateTimeZone.UTC);
        }
        final DateTime lastModified = lastModifiedTemp;



        if(matchETag != null && !matchETag.equals(copySourceETag))
            return new TextResource(HttpServletResponse.SC_PRECONDITION_FAILED, "");
        if(notMatchETag != null && notMatchETag.equals(copySourceETag))
            return new TextResource(HttpServletResponse.SC_PRECONDITION_FAILED, "");

        // TODO: last modified checks - x-amz-copy-source-if-unmodified-since and x-amz-copy-source-if-modified-since

        byte[] digest = null;
        if(copySourceBucket.equals(targetBucketName) && copySourceObject.equals(targetBlobName)) {
            if(metadataDirective != null && metadataDirective.equals("REPLACE")) {
                HashMap<String,String> md = new HashMap<>(copySourceStat.getMetadata());
                for(Map.Entry<String, String> kv : S3UserMetadataUtility.extractUserMetadata(md).entrySet()) {
                    md.remove(kv.getKey());
                }
                md.putAll(S3UserMetadataUtility.extractUserMetadata(metadataMap));
                xdi.setMetadata(token, targetDomain, targetBucketName, S3Namespace.user().blobName(targetBlobName), md).get();
            }
            digest = Hex.decodeHex(copySourceETag.toCharArray());
        } else {
            OutputStream outputStream = xdi.openForWriting(token, targetDomain, targetBucketName, S3Namespace.user().blobName(targetBlobName), metadataMap);
            DigestOutputStream digestOutputStream = new DigestOutputStream(outputStream, MessageDigest.getInstance("MD5"));
            BlobInfo blobInfo = xdi.getBlobInfo(token, S3Endpoint.FDS_S3, copySourceParts[0], S3Namespace.user().blobName(copySourceParts[1])).get();
            xdi.readToOutputStream(token, blobInfo, digestOutputStream).get();
            digestOutputStream.close();
            digest = digestOutputStream.getMessageDigest().digest();
        }

        XmlElement responseFrame = new XmlElement("CopyObjectResult")
                .withValueElt("LastModified", S3Endpoint.formatAwsDate(lastModified))
                .withValueElt("ETag", "\"" + Hex.encodeHexString(digest) + "\"");

        return new XmlResource(responseFrame.minifiedDocumentString());
    }

    private String normalizeETag(String input) {
        if(input != null)
            return input.trim().toLowerCase().replaceAll("[^0-9a-f]", "");

        return null;
    }
}
