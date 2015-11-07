package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.jets3t.service.utils.ServiceUtils;

import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.VolumeContents;
import com.formationds.xdi.Xdi;
import com.google.common.collect.Iterables;

public class ListObjects implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public ListObjects(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucket = ctx.getRouteParameter("bucket");
        
        Map<String, Collection<String>> queryParameters = ctx.getQueryParameters();
        String delimiter = Iterables.getLast(queryParameters.getOrDefault("delimiter",
                                                                          Collections.emptySet()),
                                             "");
        String prefix = Iterables.getLast(queryParameters.getOrDefault("prefix",
                                                                       Collections.emptySet()),
                                          "");

        // [FS-745] We must return 404 if the bucket doesn't exist, regardless of authentication status.
        // Amazon S3 treats the existence (or non-existence) of a bucket as a public resource.
        // XDI implements the Brewer-Nash model for volumes, so we need to bypass authorization for this
        // Tested in S3SmokeTest.testMissingBucketReturnsFourOfFour.

        if (!xdi.volumeExists(S3Endpoint.FDS_S3, bucket)) {
            return new S3Failure(S3Failure.ErrorCode.NoSuchBucket, "No such bucket", bucket);
        }

        VolumeContents volumeContentsResponse = 
                xdi.volumeContents(token,
                                   S3Endpoint.FDS_S3,
                                   bucket,
                                   Integer.MAX_VALUE,
                                   0,
                                   S3Namespace.user().blobName(prefix),
                                   BlobListOrder.UNSPECIFIED,
                                   false,
                                   delimiter == null || delimiter.isEmpty()
                                           ? PatternSemantics.PREFIX
                                           : PatternSemantics.PREFIX_AND_DELIMITER,
                                   delimiter).get();
        List<BlobDescriptor> contents = volumeContentsResponse.getBlobs();

        XmlElement result = new XmlElement("ListBucketResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Name", bucket)
                .withValueElt("Prefix", prefix)
                .withValueElt("Marker", "")
                .withValueElt("MaxKeys", Integer.toString(1000))
                .withValueElt("IsTruncated", "false");
        if (delimiter != null && !delimiter.isEmpty())
        {
            result.withValueElt("Delimiter", delimiter);
        }

        contents.stream()
                .filter(c -> S3Namespace.user().isInNamespace(c.getName()))
                .map(c -> {
                    String etag = c.getMetadata().getOrDefault("etag", "fade004009e9272f22eb90f51619431d");
                    return new XmlElement("Contents")
                            .withValueElt("Key", S3Namespace.user().localName(c.getName()))
                            .withValueElt("LastModified", ServiceUtils.formatIso8601Date(new Date()))
                            .withValueElt("ETag", "&quot;" + etag + "&quot;")
                            .withValueElt("Size", Long.toString(c.getByteCount()))
                            .withValueElt("StorageSize", "STANDARD")
                            .withElt(new XmlElement("Owner")
                                    .withValueElt("ID", UUID.randomUUID().toString())
                                    .withValueElt("DisplayName", S3Endpoint.FDS_S3));
                })
                .forEach(e -> result.withElt(e));
        
        List<String> skippedPrefixes = volumeContentsResponse.getSkippedPrefixes();
        if (!skippedPrefixes.isEmpty())
        {
            XmlElement commonPrefixes = new XmlElement("CommonPrefixes");
            for (String skippedPrefix : skippedPrefixes)
            {
                commonPrefixes.withElt(new XmlElement("Prefix", skippedPrefix));
            }
            result.withElt(commonPrefixes);
        }
        
        return new XmlResource(result.minifiedDocumentString());
    }
}
