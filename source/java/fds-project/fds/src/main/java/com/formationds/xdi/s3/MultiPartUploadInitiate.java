package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.AuthenticatedXdi;
import com.formationds.xdi.io.BlobSpecifier;

import java.util.Map;
import java.util.UUID;

public class MultiPartUploadInitiate implements SyncRequestHandler {
    private AuthenticatedXdi xdi;
    private AuthenticationToken token;

    public MultiPartUploadInitiate(AuthenticatedXdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucket = ctx.getRouteParameter("bucket");
        String objectName = ctx.getRouteParameter("object");
        UUID txid = UUID.randomUUID();

        BlobSpecifier specifier = new BlobSpecifier(S3Endpoint.FDS_S3, bucket, S3Namespace.user().blobName(objectName));
        Map<String, String> userMetadata = S3MetadataUtility.requestUserMetadata(ctx);
        xdi.multipart(token, specifier, txid.toString()).initiate(userMetadata).get();

        XmlElement response = new XmlElement("InitiateMultipartUploadResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Bucket", bucket)
                .withValueElt("Key", objectName)
                .withValueElt("UploadId", txid.toString());

        return new XmlResource(response.documentString());
    }
}
