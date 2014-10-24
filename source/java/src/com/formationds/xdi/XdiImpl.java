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

public class XdiImpl implements Xdi {
    private final AmService.Iface am;
    private Authenticator authenticator;
    private Authorizer authorizer;
    private ConfigurationService.Iface config;
    private FDSP_ConfigPathReq.Iface legacyConfig;

    public XdiImpl(AmService.Iface am, ConfigurationService.Iface config, Authenticator authenticator, Authorizer authorizer, FDSP_ConfigPathReq.Iface legacyConfig) {
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
    @Override
    public long createVolume(AuthenticationToken token, String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumePolicy, authorizer.tenantId(token));
        SetVolumeQosParams.setVolumeQos(legacyConfig, volumeName, 0, 10, 0);
      /**
       * allows the UI to assign a snapshot policy to a volume without having to make an
       * extra call.
       */
      return config.getVolumeId( volumeName );
    }

    @Override
    public void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        config.deleteVolume(domainName, volumeName);
    }

    @Override
    public VolumeStatus statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.volumeStatus(domainName, volumeName);
    }

    @Override
    public VolumeDescriptor volumeConfiguration(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return config.statVolume(domainName, volumeName);
    }

    @Override
    public List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws ApiException, TException {
        return config.listVolumes(domainName)
                .stream()
                .filter(v -> authorizer.hasAccess(token, v.getName()))
                .collect(Collectors.toList());
    }

    @Override
    public List<BlobDescriptor> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    @Override
    public BlobDescriptor statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.statBlob(domainName, volumeName, blobName);
    }

    @Override
    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception {
        attemptVolumeAccess(token, volumeName);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    @Override
    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        attemptVolumeAccess(token, volumeName);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }

    @Override
    public byte[] writeStream(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName);
        VolumeDescriptor volume = config.statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        return new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    @Override
    public void deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        am.deleteBlob(domainName, volumeName, blobName);
    }

    @Override
    public Authenticator getAuthenticator() {
        return authenticator;
    }

    @Override
    public Authorizer getAuthorizer() {
        return authorizer;
    }

    @Override
    public String getSystemVolumeName(AuthenticationToken token) throws SecurityException {
        long tenantId = authorizer.tenantId(token);
        return "SYSTEM_VOLUME_" + tenantId;
    }
}
