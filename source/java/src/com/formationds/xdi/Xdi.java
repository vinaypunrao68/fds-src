/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.OMConfigException;
import com.formationds.util.thrift.OMConfigServiceClient;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import java.io.InputStream;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class Xdi {

    public static final String LAST_MODIFIED = "Last-Modified";

    private final AmService.Iface am;
    private Authenticator authenticator;
    private Authorizer authorizer;
    private OMConfigServiceClient config;

    public Xdi(AmService.Iface am, OMConfigServiceClient config, Authenticator authenticator,
               Authorizer authorizer) {
        this.am = am;
        this.config = config;
        this.authenticator = authenticator;
        this.authorizer = authorizer;
    }

    private void attemptVolumeAccess(AuthenticationToken token, String volumeName) throws SecurityException {
        if (!authorizer.hasAccess(token, volumeName)) {
            throw new SecurityException();
        }
    }

    public long createVolume(AuthenticationToken token, String domainName, String volumeName,
                             VolumeSettings volumePolicy) throws ApiException, TException {

        try {
            config.createVolume(token, domainName, volumeName, volumePolicy, authorizer.tenantId(token));

            /**
             * allows the UI to assign a snapshot policy to a volume without having to make an
             * extra call.
             */
            return config.getVolumeId(token, domainName, volumeName);
        } catch (OMConfigException e) {
            throw new TException("OM Client create volume failed.", e);
        }

    }

    public void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        try {
            config.deleteVolume(token, domainName, volumeName);
        } catch (OMConfigException e) {
            throw new TException("OM Client delete volume failed.", e);
        }
    }

    public VolumeStatus statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        return am.volumeStatus(domainName, volumeName);
    }

    public VolumeDescriptor volumeConfiguration(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName);
        try {
            return config.statVolume(token, domainName, volumeName);
        } catch (OMConfigException e) {
            throw new TException("OM Client volume config request failed.", e);
        }
    }

    public List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws ApiException, TException {
        try {
            return config.listVolumes(token, domainName)
                         .stream()
                         .filter(v -> authorizer.hasAccess(token, v.getName()))
                         .collect(Collectors.toList());
        } catch (OMConfigException e) {
            throw new TException("OM Client listVolumes failed.", e);
        }
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
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(token, domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        attemptVolumeAccess(token, volumeName);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(token, domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }

    public byte[] writeStream(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName);
        VolumeDescriptor volume = config.statVolume(token, domainName, volumeName);
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

    public void setMetadata(AuthenticationToken token, String domain, String volume, String blob, HashMap<String, String> metadataMap) throws TException {
        attemptVolumeAccess(token, volume);
        TxDescriptor tx = am.startBlobTx(domain, volume, blob, 0);
        am.updateMetadata(domain, volume, blob, tx, metadataMap);
        am.commitBlobTx(domain, volume, blob, tx);

    }
}
