package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.protocol.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.StaticFileHandler;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.swift.SwiftUtility;
import org.joda.time.DateTime;

import java.util.List;
import java.util.Map;

public class HeadObject implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public HeadObject(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext contex) throws Exception {
        String volume = contex.getRouteParameter("bucket");
        String object = contex.getRouteParameter("object");

        BlobDescriptor stat = xdi.statBlob(token, S3Endpoint.FDS_S3, volume, S3Namespace.user().blobName(object)).get();
        Map<String, String> metadata = stat.getMetadata();

        List<MultipartUpload.PartInfo> partInfoList = MultipartUpload.getPartInfoList(stat);
        long length;
        if(partInfoList != null)
            length = MultipartUpload.PartInfo.computeLength(partInfoList);
        else
            length = stat.byteCount;

        String contentType = metadata.getOrDefault("Content-Type", StaticFileHandler.getMimeType(object));
        String lastModified = metadata.getOrDefault("Last-Modified", SwiftUtility.formatRfc1123Date(DateTime.now()));
        String etag = "\"" + metadata.getOrDefault("etag", "") + "\"";
        Resource result = new TextResource("")
                .withContentType(contentType)
                .withHeader("Content-Length", Long.toString(length))
                .withHeader("Last-Modified", lastModified)
                .withHeader("ETag", etag);

        for(Map.Entry<String, String> entry : S3UserMetadataUtility.extractUserMetadata(metadata).entrySet())
            result = result.withHeader(entry.getKey(), entry.getValue());

        return result;
    }
}
