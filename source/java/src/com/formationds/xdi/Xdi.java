package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import java.io.InputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class Xdi {
    private final AmService.Iface am;
    private ConfigurationService.Iface config;

    public static final String LAST_MODIFIED = "Last-Modified";

    public Xdi(AmService.Iface am, ConfigurationService.Iface config) {
        this.am = am;
        this.config = config;
    }

    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumePolicy);
    }

    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        config.deleteVolume(domainName, volumeName);
    }

    public VolumeDescriptor statVolume(String domainName, String volumeName) throws ApiException, TException {
        return config.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return config.listVolumes(domainName);
    }

    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        return am.statBlob(domainName, volumeName, blobName);
    }

    public InputStream readStream(String domainName, String volumeName, String blobName) throws Exception {
        Iterator<byte[]> iterator = new BlockIterator(am, config).read(domainName, volumeName, blobName);
        return new BlockStreamer(iterator);
    }

    public void writeStream(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        VolumeDescriptor volume = statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        am.deleteBlob(domainName, volumeName, blobName);
    }
}
