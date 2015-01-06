package com.formationds.xdi;

import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.async.CompletableFutureUtility;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */

public class AuthorizedAsyncAm implements AsyncAm {
    private AuthenticationToken token;
    private Authorizer authorizer;
    private AsyncAm asyncAm;

    public AuthorizedAsyncAm(AuthenticationToken token, Authorizer authorizer, AsyncAm asyncAm) {
        this.token = token;
        this.authorizer = authorizer;
        this.asyncAm = asyncAm;
    }

    @Override
    public void start() throws Exception {
        asyncAm.start();
    }

    @Override
    public CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }
        return asyncAm.attachVolume(domainName, volumeName);
    }

    @Override
    public CompletableFuture<List<BlobDescriptor>> volumeContents(String domainName, String volumeName, int count, long offset) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.volumeContents(domainName, volumeName, count, offset);
    }

    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.statBlob(domainName, volumeName, blobName);
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.getBlobWithMeta(domainName, volumeName, blobName, length, offset);
    }

    @Override
    public CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.startBlobTx(domainName, volumeName, blobName, blobMode);
    }

    @Override
    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.commitBlobTx(domainName, volumeName, blobName, txDescriptor);
    }

    @Override
    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.abortBlobTx(domainName, volumeName, blobName, txDescriptor);
    }

    @Override
    public CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.getBlob(domainName, volumeName, blobName, length, offset);
    }

    @Override
    public CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, Map<String, String> metadata) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.updateMetadata(domainName, volumeName, blobName, txDescriptor, metadata);
    }

    @Override
    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.updateBlob(domainName, volumeName, blobName, txDescriptor, bytes, length, objectOffset, isLast);
    }

    @Override
    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName, int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.updateBlobOnce(domainName, volumeName, blobName, blobMode, bytes, length, offset, metadata);
    }

    @Override
    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.deleteBlob(domainName, volumeName, blobName);
    }

    @Override
    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

        return asyncAm.volumeStatus(domainName, volumeName);
    }
}
