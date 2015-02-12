/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import javax.security.auth.login.LoginException;
import java.io.InputStream;
import java.util.*;

public class Xdi {
    public static final String LAST_MODIFIED = "Last-Modified";

    private final AmService.Iface am;
    private ConfigurationApi config;
    private XdiAuthorizer authorizer;

    public Xdi(AmService.Iface am, ConfigurationApi config, Authenticator authenticator, Authorizer authorizer, AsyncAm asyncAm) {
        this.am = am;
        this.config = config;
        this.authorizer = new XdiAuthorizer(authenticator, authorizer, asyncAm, config);
    }

    private void attemptVolumeAccess(AuthenticationToken token, String volumeName, Intent intent) throws SecurityException {
        if (!authorizer.hasVolumePermission(token, volumeName, intent)) {
            throw new SecurityException();
        }
    }

    private void attemptToplevelAccess(AuthenticationToken token, Intent intent) throws SecurityException {
        if (!authorizer.hasToplevelPermission(token, intent)) {
            throw new SecurityException();
        }
    }

    public boolean volumeExists(String domainName, String volumeName) throws ApiException, TException {
        try {
            return (config.statVolume(domainName, volumeName) != null);
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                return false;
            } else {
                throw e;
            }
        }
    }

    public long createVolume(AuthenticationToken token, String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException {
        attemptToplevelAccess(token, Intent.readWrite);
        config.createVolume(domainName, volumeName, volumePolicy, authorizer.tenantId(token));
        return config.getVolumeId(volumeName);
    }

    // TODO: this is ignoring case where volume doesn't exist.  s3 apis may require the return of a
    // specific error code indicating the bucket does not exist.
    public void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        if (config.statVolume(domainName, volumeName) != null) {
            attemptVolumeAccess(token, volumeName, Intent.delete);
            config.deleteVolume(domainName, volumeName);
        }
    }

    public VolumeStatus statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return am.volumeStatus(domainName, volumeName);
    }

    public VolumeDescriptor volumeConfiguration(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return config.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws ApiException, TException {
        attemptToplevelAccess(token, Intent.read);
        List<VolumeDescriptor> volumes = config.listVolumes(domainName);
        List<VolumeDescriptor> result = new ArrayList<>();
        for (VolumeDescriptor volume : volumes) {
            if (authorizer.hasVolumePermission(token, volume.getName(), Intent.read)) {
                result.add(volume);
            }
        }
        return result;
    }

    public List<BlobDescriptor> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset, String pattern, BlobListOrder orderBy, boolean descending) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return am.volumeContents(domainName, volumeName, count, offset, pattern, orderBy, descending);
    }

    public BlobDescriptor statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return am.statBlob(domainName, volumeName, blobName);
    }

    public void updateMetadata(AuthenticationToken token, String domainName, String volumeName, String blobName, Map<String, String> metadata) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.readWrite);
        TxDescriptor txDescriptor = am.startBlobTx(domainName, volumeName, blobName, 0);
        am.updateMetadata(domainName, volumeName, blobName, txDescriptor, metadata);
        am.commitBlobTx(domainName, volumeName, blobName, txDescriptor);
    }

    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception {
        attemptVolumeAccess(token, volumeName, Intent.read);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    public InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        attemptVolumeAccess(token, volumeName, Intent.read);
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }

    public byte[] writeStream(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName, Intent.readWrite);
        VolumeDescriptor volume = config.statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        return new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    public void deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.delete);
        am.deleteBlob(domainName, volumeName, blobName);
    }

    public XdiAuthorizer getAuthorizer() {
        return authorizer;
    }

    public String getSystemVolumeName(AuthenticationToken token) throws SecurityException {
        long tenantId = authorizer.tenantId(token);
        return XdiConfigurationApi.systemFolderName(tenantId);
    }

    public void setMetadata(AuthenticationToken token, String domain, String volume, String blob, HashMap<String, String> metadataMap) throws TException {
        attemptVolumeAccess(token, volume, Intent.readWrite);
        TxDescriptor tx = am.startBlobTx(domain, volume, blob, 0);
        am.updateMetadata(domain, volume, blob, tx, metadataMap);
        am.commitBlobTx(domain, volume, blob, tx);

    }

    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return authorizer.authenticate(login, password);
    }
}
