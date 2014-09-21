package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import com.formationds.apis.*;
import com.formationds.om.rest.SetVolumeQosParams;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import java.io.InputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class Xdi {
    private final AmService.Iface am;
    private ConfigurationService.Iface config;

    public static final String LAST_MODIFIED = "Last-Modified";
    private Authenticator authenticator;
    private Authorizer authorizer;
    private FDSP_ConfigPathReq.Iface legacyConfig;

    public Xdi(AmService.Iface am, ConfigurationService.Iface config, Authenticator authenticator, Authorizer authorizer, FDSP_ConfigPathReq.Iface legacyConfig) {
        this.am = am;
        this.config = config;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.legacyConfig = legacyConfig;
    }

    private void attemptVolumeAccess(AuthenticationToken token, String volumeName) throws SecurityException {
        if (!authorizer.hasAccess(token, volumeName)) {
            throw new SecurityException();
        }
    }
    public void createVolume(AuthenticationToken token, String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumePolicy, authorizer.tenantId(token));
        SetVolumeQosParams.setVolumeQos(legacyConfig, volumeName, 0, 10, 0);
    }

    public void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        config.deleteVolume(domainName, volumeName);
    }

    public VolumeStatus statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.volumeStatus(domainName, volumeName);
    }

    public VolumeDescriptor volumeConfiguration(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return config.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws ApiException, TException {
        return config.listVolumes(domainName)
                .stream()
                .filter(v -> authorizer.hasAccess(token, v.getName()))
                .collect(Collectors.toList());
    }

    public List<BlobDescriptor> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    public BlobDescriptor statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.statBlob(domainName, volumeName, blobName);
    }

    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception {
        attemptVolumeAccess(token, volumeName);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        attemptVolumeAccess(token, volumeName);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }

    public byte[] writeStream(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName);
        VolumeDescriptor volume = config.statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        return new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    public void deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        am.deleteBlob(domainName, volumeName, blobName);
    }

    public Authenticator getAuthenticator() {
        return authenticator;
    }

    public Authorizer getAuthorizer() {
        return authorizer;
    }

    public String getSystemVolumeName(AuthenticationToken token) throws SecurityException {
        long tenantId = authorizer.tenantId(token);
        return "SYSTEM_VOLUME_" + tenantId;
    }
}
