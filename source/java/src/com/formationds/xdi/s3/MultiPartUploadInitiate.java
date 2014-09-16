package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.apis.ErrorCode;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.XmlElement;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.XmlResource;
import com.formationds.xdi.Xdi;
import org.eclipse.jetty.server.Request;

import java.util.Map;
import java.util.UUID;

public class MultiPartUploadInitiate implements RequestHandler {
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
                xdi.createVolume(token, S3Endpoint.FDS_S3_SYSTEM, systemVolume, new VolumeSettings(2 * 1024 * 1024, VolumeType.OBJECT, 0));
            } catch (ApiException ex) {
                if (ex.getErrorCode() != ErrorCode.RESOURCE_ALREADY_EXISTS)
                    throw ex;
            }
        }
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String bucket = requiredString(routeParameters, "bucket");
        String objectName = requiredString(routeParameters, "object");
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
