package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;

import java.util.UUID;

public class MultiPartUploadInitiate implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public MultiPartUploadInitiate(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    private void ensureSystemBucketCreated() throws Exception {
        String systemVolume = xdi.getSystemVolumeName(token);
        boolean exists = xdi.listVolumes(token, S3Endpoint.FDS_S3_SYSTEM).stream()
                .anyMatch(o -> o.getName().equals(systemVolume));

        if (!exists) {
            try {
                xdi.createVolume(token, S3Endpoint.FDS_S3_SYSTEM, systemVolume, new VolumeSettings(2 * 1024 * 1024, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY));
            } catch (ApiException ex) {
                if (ex.getErrorCode() != ErrorCode.RESOURCE_ALREADY_EXISTS)
                    throw ex;
            }
        }
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        String bucket = ctx.getRouteParameter("bucket");
        String objectName = ctx.getRouteParameter("object");
        UUID txid = UUID.randomUUID();
        ensureSystemBucketCreated();

        XmlElement response = new XmlElement("InitiateMultipartUploadResult")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/")
                .withValueElt("Bucket", bucket)
                .withValueElt("Key", objectName)
                .withValueElt("UploadId", txid.toString());

        return new XmlResource(response.documentString());
    }
}
