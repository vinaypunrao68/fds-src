package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.*;
import com.formationds.om.rest.SetVolumeQosParams;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import javax.security.auth.login.LoginException;
import java.io.InputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class Xdi {
    private final AmService.Iface am;
    private ConfigurationService.Iface config;

    public static final String LAST_MODIFIED = "Last-Modified";
    private Authenticator authenticator;
    private FDSP_ConfigPathReq.Iface legacyConfig;

    public Xdi(AmService.Iface am, ConfigurationService.Iface config, Authenticator authenticator, FDSP_ConfigPathReq.Iface legacyConfig) {
        this.am = am;
        this.config = config;
        this.authenticator = authenticator;
        this.legacyConfig = legacyConfig;
    }

    public void createVolume(String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumePolicy, 0);
        SetVolumeQosParams.setVolumeQos(legacyConfig, volumeName, 0, 10, 0);
    }

    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        config.deleteVolume(domainName, volumeName);
    }

    public VolumeStatus statVolume(String domainName, String volumeName) throws ApiException, TException {
        return am.volumeStatus(domainName, volumeName);
    }

    public VolumeDescriptor volumeConfiguration(String domainName, String volumeName) throws ApiException, TException {
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
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    public InputStream readStream(String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }

    public byte[] writeStream(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        VolumeDescriptor volume = config.statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        return new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        am.deleteBlob(domainName, volumeName, blobName);
    }

    public AuthenticationToken resolveToken(String signature) throws LoginException {
        return authenticator.resolveToken(signature);
    }

    public AuthenticationToken reissueToken(String login, String password) throws LoginException {
        return authenticator.reissueToken(login, password);
    }
}
