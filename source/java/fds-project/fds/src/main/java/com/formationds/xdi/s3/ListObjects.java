package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.services.s3.internal.ServiceUtils;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;

import java.util.Date;
import java.util.List;
import java.util.UUID;

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

        // [FS-745] We must return 404 if the bucket doesn't exist, regardless of authentication status.
        // Amazon S3 treats the existence (or non-existence) of a bucket as a public resource.
        // XDI implements the Brewer-Nash model for volumes, so we need to bypass authorization for this
        // Tested in S3SmokeTest.testMissingBucketReturnsFourOfFour.

        if (!xdi.volumeExists(S3Endpoint.FDS_S3, bucket)) {
            return new S3Failure(S3Failure.ErrorCode.NoSuchBucket, "No such bucket", bucket);
        }

        List<BlobDescriptor> contents = xdi.volumeContents(token, S3Endpoint.FDS_S3, bucket, Integer.MAX_VALUE, 0, "", BlobListOrder.UNSPECIFIED, false).get();

        XmlElement result = new XmlElement("ListBucketResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Name", bucket)
                .withValueElt("Prefix", "")
                .withValueElt("Marker", "")
                .withValueElt("MaxKeys", Integer.toString(1000))
                .withValueElt("IsTruncated", "false");

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
        return new XmlResource(result.minifiedDocumentString());
    }
}
