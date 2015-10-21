/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.io.BlobSpecifier;
import com.formationds.xdi.s3.MultipartUpload;
import com.formationds.xdi.security.Intent;
import com.formationds.xdi.security.XdiAuthorizer;

import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.function.Supplier;

public class Xdi {
    public static final String LAST_MODIFIED = "Last-Modified";
    private ConfigurationApi config;
    private AsyncAm asyncAm;
    private Supplier<AsyncStreamer> factory;
    private XdiAuthorizer authorizer;

    public Xdi(ConfigurationApi config, Authenticator authenticator, Authorizer authorizer, AsyncAm asyncAm, Supplier<AsyncStreamer> factory) {
        this.config = config;
        this.asyncAm = asyncAm;
        this.factory = factory;
        this.authorizer = new XdiAuthorizer(authenticator, authorizer, asyncAm);
    }

    private void attemptVolumeAccess(AuthenticationToken token, String volumeName, Intent intent) throws SecurityException {
        if (!authorizer.hasVolumePermission(token, volumeName, intent)) {
            throw new SecurityException();
        }
    }

    private void attemptToplevelAccess(AuthenticationToken token) throws SecurityException {
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
        attemptToplevelAccess(token);
        config.createVolume(domainName, volumeName, volumePolicy, authorizer.tenantId(token));
        return config.getVolumeId(volumeName);
    }

    // TODO: this is ignoring case where volume doesn't exist.  s3 apis may require the return of a
    // specific error code indicating the bucket does not exist.
    public void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException {
        try {
            if (config.statVolume(domainName, volumeName) != null) {
                attemptVolumeAccess(token, volumeName, Intent.delete);
                config.deleteVolume(domainName, volumeName);
            }
        } catch(ApiException ex) {
            if(ex.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                return;
            throw ex;
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
        attemptToplevelAccess(token);
        List<VolumeDescriptor> volumes = config.listVolumes(domainName);
        List<VolumeDescriptor> result = new ArrayList<>();
        for (VolumeDescriptor volume : volumes) {
            if (authorizer.hasVolumePermission(token, volume.getName(), Intent.read)) {
                result.add(volume);
            }
        }
        return result;
    }

    public CompletableFuture<VolumeContents> volumeContents(AuthenticationToken token,
                                                                  String domainName,
                                                                  String volumeName,
                                                                  int count,
                                                                  long offset,
                                                                  String pattern,
                                                                  BlobListOrder orderBy,
                                                                  boolean descending,
                                                                  PatternSemantics patternSemantics,
                                                                  String delimiter)
              throws ApiException, TException
    {
        attemptVolumeAccess(token, volumeName, Intent.read);
        return asyncAm.volumeContents(domainName,
                                      volumeName,
                                      count,
                                      offset,
                                      pattern,
                                      patternSemantics,
                                      delimiter,
                                      orderBy,
                                      descending);
    }

    public CompletableFuture<BlobDescriptor> statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        CompletableFuture<BlobDescriptor> cf = new CompletableFuture<>();

        attemptBlobAccess(token, domainName, volumeName, blobName, Intent.read)
                .handle((obd, t) -> {
                    if (t != null) {
                        cf.completeExceptionally(t);
                    }

                    if (obd.isPresent()) {
                        cf.complete(obd.get());
                    } else {
                        cf.completeExceptionally(new ApiException("Missing resource", ErrorCode.MISSING_RESOURCE));
                    }
                    return 0;
                });

        return cf;
    }

    /**
     * Attempt accessing this blob, optionally returning the corresponding blob descriptor if it exists.
     */
    private CompletableFuture<Optional<BlobDescriptor>> attemptBlobAccess(AuthenticationToken token, String domainName, String volumeName, String blobName, Intent intent) throws TException {
        CompletableFuture<Optional<BlobDescriptor>> cf = new CompletableFuture<>();

        asyncAm.statBlob(domainName, volumeName, blobName)
                .handle((bd, t) -> {
                    Map<String, String> metadata = new HashMap<>();

                    if (bd != null) {
                        metadata = bd.getMetadata();
                    }

                    if (!authorizer.hasBlobPermission(token, volumeName, intent, metadata)) {
                        cf.completeExceptionally(new SecurityException());
                    } else {
                        if (bd == null) {
                            cf.complete(Optional.<BlobDescriptor>empty());
                        } else {
                            cf.complete(Optional.of(bd));
                        }
                    }
                    return 0;
                });
        return cf;
    }

    public CompletableFuture<BlobInfo> getBlobInfo(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception {
        CompletableFuture<BlobInfo> cf = new CompletableFuture<>();

        factory.get().getBlobInfo(domainName, volumeName, blobName)
                .handle((bi, t) -> {
                    if (t != null) {
                        cf.completeExceptionally(t);
                    } else {
                        if (!authorizer.hasBlobPermission(token, volumeName, Intent.read, bi.getBlobDescriptor().getMetadata())) {
                            cf.completeExceptionally(new SecurityException());
                        } else {
                            cf.complete(bi);
                        }
                    }
                    return 0;
                });

        return cf;
    }

    public CompletableFuture<Void> readToOutputStream(AuthenticationToken token, BlobInfo blobInfo, OutputStream out, long offset, long length) {
        if (!authorizer.hasBlobPermission(token, blobInfo.getVolume(), Intent.read, blobInfo.getBlobDescriptor().getMetadata())) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }
        return factory.get().readToOutputStream(blobInfo, out, offset, length);
    }

    public CompletableFuture<Void> readToOutputStream(AuthenticationToken token, BlobInfo blobInfo, OutputStream out) throws Exception {
        return this.readToOutputStream(token, blobInfo, out, 0, blobInfo.getBlobDescriptor().getByteCount());
    }

    public OutputStream openForWriting(AuthenticationToken token, String domainName, String volumeName, String blobName, Map<String, String> metadata) throws Exception {
        Map<String, String> updatedMetadata = new HashMap<>();
        updatedMetadata.putAll(metadata);
        try {
            Optional<BlobDescriptor> blobDescriptor = attemptBlobAccess(token, domainName, volumeName, blobName, Intent.readWrite).get();
            if (blobDescriptor.isPresent()) {
                updatedMetadata.putAll(blobDescriptor.get().getMetadata());
            }
        } catch (ExecutionException e) {
            if (e.getCause() instanceof SecurityException) {
                throw (SecurityException) e.getCause();
            }
        }
        return factory.get().openForWriting(domainName, volumeName, blobName, updatedMetadata);
    }

    public CompletableFuture<PutResult> put(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) {
        try {
            return attemptBlobAccess(token, domainName, volumeName, blobName, Intent.readWrite)
                    .thenCompose(bd -> factory.get().putBlobFromStream(domainName, volumeName, blobName, metadata, in));
        } catch (TException e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException {
        attemptBlobAccess(token, domainName, volumeName, blobName, Intent.delete);
        return asyncAm.deleteBlob(domainName, volumeName, blobName);
    }

    public XdiAuthorizer getAuthorizer() {
        return authorizer;
    }

    public CompletableFuture<Void> setMetadata(AuthenticationToken token, String domain, String volume, String blob, HashMap<String, String> metadataMap) throws TException {
        attemptBlobAccess(token, domain, volume, blob, Intent.readWrite);
    	// Note: explicit casts added to workaround issues with type inference in Eclipse compiler
        return asyncAm.startBlobTx(domain, volume, blob, 1)
                .thenCompose(tx -> asyncAm.updateMetadata(domain, volume, blob, tx, metadataMap).thenApply(x -> tx))
                .thenCompose(tx -> asyncAm.commitBlobTx(domain, volume, blob, (TxDescriptor)tx));
    }

    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return authorizer.authenticate(login, password);
    }

    public MultipartUpload multipart(AuthenticationToken token, BlobSpecifier specifier, String txid) throws TException, ExecutionException, InterruptedException {
        Optional<BlobDescriptor> blobDescriptor = attemptBlobAccess(token, specifier.getDomainName(), specifier.getVolumeName(), specifier.getBlobName(), Intent.readWrite).get();
        VolumeDescriptor stat = config.statVolume(specifier.getDomainName(), specifier.getVolumeName());
        return new MultipartUpload(specifier, blobDescriptor, asyncAm, stat.getPolicy().getMaxObjectSizeInBytes(), txid);
    }

    public CompletableFuture<Void> readMultipart(AuthenticationToken token, BlobInfo blobInfo, OutputStream stream) {
        List<MultipartUpload.PartInfo> partInfoList = MultipartUpload.getPartInfoList(blobInfo.getBlobDescriptor());
        if(partInfoList == null)
            throw new IllegalArgumentException("blob is not a multipart blob");

        if (!authorizer.hasBlobPermission(token, blobInfo.getVolume(), Intent.read, blobInfo.getBlobDescriptor().getMetadata())) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return MultipartUpload.read(asyncAm, new BlobSpecifier(blobInfo.getDomain(), blobInfo.getVolume(), blobInfo.getBlob()), partInfoList, blobInfo.getVolumeDescriptor().getPolicy().getMaxObjectSizeInBytes())
                .readToStream(stream);
    }
}
