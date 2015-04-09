/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.XdiService;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.ErrorCode;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;
import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

public class Xdi {
    public static final String LAST_MODIFIED = "Last-Modified";
    private final XdiService.Iface am;
    private ConfigurationApi config;
    private AsyncAm asyncAm;
    private Supplier<AsyncStreamer> factory;
    private XdiAuthorizer authorizer;

    public Xdi(XdiService.Iface am, ConfigurationApi config, Authenticator authenticator, Authorizer authorizer, AsyncAm asyncAm, Supplier<AsyncStreamer> factory) {
        this.am = am;
        this.config = config;
        this.asyncAm = asyncAm;
        this.factory = factory;
        this.authorizer = new XdiAuthorizer(authenticator, authorizer, asyncAm, config);
    }

    private void attemptVolumeAccess(AuthenticationToken token, String volumeName, Intent intent) throws SecurityException {
        if (!authorizer.hasVolumePermission(token, volumeName, intent)) {
            throw new SecurityException();
        }
    }

    private void attemptToplevelAccess(AuthenticationToken token, Intent intent) throws SecurityException {
        if (!authorizer.hasToplevelPermission(token)) {
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

    public CompletableFuture<VolumeStatus> statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return asyncAm.volumeStatus(domainName, volumeName);
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

    public CompletableFuture<List<BlobDescriptor>> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset, String pattern, BlobListOrder orderBy, boolean descending) throws ApiException, TException {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return asyncAm.volumeContents(domainName, volumeName, count, offset, pattern, orderBy, descending);
    }

    public BlobDescriptor statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        return attemptBlobAccess(token, domainName, volumeName, blobName, Intent.read);
    }

    private BlobDescriptor attemptBlobAccess(AuthenticationToken token, String domainName, String volumeName, String blobName, Intent intent) throws TException {
        Map<String, String> metadata = new HashMap<>();

        BlobDescriptor blobDescriptor = am.statBlob(domainName, volumeName, blobName);
        metadata.putAll(blobDescriptor.getMetadata());

        if (!authorizer.hasBlobPermission(token, volumeName, intent, metadata)) {
            throw new SecurityException();
        }
        return blobDescriptor;
    }

    public CompletableFuture<BlobInfo> getBlobInfo(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception {
        attemptBlobAccess(token, domainName, volumeName, blobName, Intent.read);
        return factory.get().getBlobInfo(domainName, volumeName, blobName);
    }

    public CompletableFuture<Void> readToOutputStream(AuthenticationToken token, BlobInfo blobInfo, OutputStream out, long offset, long length) throws Exception {
        attemptBlobAccess(token, blobInfo.getDomain(), blobInfo.getVolume(), blobInfo.getBlob(), Intent.read);
        return factory.get().readToOutputStream(blobInfo, out, offset, length);
    }

    public CompletableFuture<Void> readToOutputStream(AuthenticationToken token, BlobInfo blobInfo, OutputStream out) throws Exception {
        return this.readToOutputStream(token, blobInfo, out, 0, blobInfo.getBlobDescriptor().byteCount);
    }

    public OutputStream openForWriting(AuthenticationToken token, String domainName, String volumeName, String blobName, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName, Intent.readWrite);
        return factory.get().openForWriting(domainName, volumeName, blobName, metadata);
    }

    public CompletableFuture<AsyncStreamer.PutResult> put(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        attemptVolumeAccess(token, volumeName, Intent.readWrite);
        return factory.get().putBlobFromStream(domainName, volumeName, blobName, metadata, in);
    }

    public CompletableFuture<Void> deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptBlobAccess(token, domainName, volumeName, blobName, Intent.delete);
        return asyncAm.deleteBlob(domainName, volumeName, blobName);
    }

    public XdiAuthorizer getAuthorizer() {
        return authorizer;
    }

    public String getSystemVolumeName(AuthenticationToken token) throws SecurityException {
        long tenantId = authorizer.tenantId(token);
        return XdiConfigurationApi.systemFolderName(tenantId);
    }

    public CompletableFuture<Void> setMetadata(AuthenticationToken token, String domain, String volume, String blob, HashMap<String, String> metadataMap) throws TException {
        attemptBlobAccess(token, domain, volume, blob, Intent.readWrite);
        return asyncAm.startBlobTx(domain, volume, blob, 1)
                .thenCompose(tx -> asyncAm.updateMetadata(domain, volume, blob, tx, metadataMap).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.commitBlobTx(domain, volume, blob, tx));
    }

    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return authorizer.authenticate(login, password);
    }
}
