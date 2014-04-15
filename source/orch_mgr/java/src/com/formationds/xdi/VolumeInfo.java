package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;

import java.util.UUID;

public class VolumeInfo {
    private String domainName;
    private String volumeName;
    private int objectSizeInBytes;
    private DateTime creationTime;
    private UUID uuid;

    public VolumeInfo(String domainName, String volumeName, int objectSizeInBytes, DateTime creationTime, String uuid) {
        this.domainName = domainName;
        this.volumeName = volumeName;
        this.objectSizeInBytes = objectSizeInBytes;
        this.creationTime = creationTime;
        this.uuid = uuid;
    }

    public DateTime getCreationTime() {
        return creationTime;
    }

    public UUID getUuid() {
        return uuid;
    }

    public String getDomainName() {
        return domainName;
    }

    public String getVolumeName() {
        return volumeName;
    }

    public int getObjectSizeInBytes() {
        return objectSizeInBytes;
    }
}
