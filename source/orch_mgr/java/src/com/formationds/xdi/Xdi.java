package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;

import java.util.UUID;

public class Xdi {
    private String domainName;
    private AmShim.Iface am;

    public Xdi(String domainName, AmShim.Iface am) {
        this.domainName = domainName;
        this.am = am;
    }

    public void createVolume(String bucketName, int blockSizeInBytes) throws Exception {
        am.createVolume(domainName, bucketName, blockSizeInBytes);
    }

    public VolumeInfo statVolume(String volumeName) throws Exception {
        VolumeDescriptor descriptor = am.statVolume(domainName, volumeName);
        return new VolumeInfo(domainName,
                volumeName,
                descriptor.getObjectSizeInBytes(),
                new DateTime(descriptor.getDateCreated()),
                new UUID());
    }
}
