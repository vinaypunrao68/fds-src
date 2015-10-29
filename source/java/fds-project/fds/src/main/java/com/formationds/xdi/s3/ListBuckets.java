package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.services.s3.internal.ServiceUtils;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.svc.types.ResourceState;
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

public class ListBuckets implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public ListBuckets(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext ctx) throws Exception {
        List<VolumeDescriptor> volumeDescriptors = xdi.listVolumes(token, S3Endpoint.FDS_S3);
        XmlElement frame = new XmlElement("ListAllMyBucketResults")
                .withAttr("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

        frame.withElt(new XmlElement("Owner")
                .withValueElt("ID", UUID.randomUUID().toString()))
                .withValueElt("DisplayName", S3Endpoint.FDS_S3);

        volumeDescriptors.stream()
                .filter(v -> v.getState() == ResourceState.Active)
                .forEach(v -> frame.withElt(
                        new XmlElement("Bucket")
                                .withValueElt("Name", v.getName())
                                .withValueElt("CreationDate", ServiceUtils.formatIso8601Date(new Date(v.getDateCreated())))
                                .withValueElt("ID", UUID.randomUUID().toString())));

        return new XmlResource(frame.minifiedDocumentString());
    }
}
