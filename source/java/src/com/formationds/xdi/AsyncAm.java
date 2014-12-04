package com.formationds.xdi;

import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.security.AuthenticationToken;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public interface AsyncAm {
    void start() throws Exception;

    CompletableFuture<Void> attachVolume(AuthenticationToken token, String domainName, String volumeName) throws TException;

    CompletableFuture<List<BlobDescriptor>> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset);

    CompletableFuture<BlobDescriptor> statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName);

    CompletableFuture<BlobWithMetadata> getBlobWithMeta(AuthenticationToken token, String domainName, String volumeName, String blobName, int length, ObjectOffset offset);

    // This one
    CompletableFuture<TxDescriptor> startBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, int blobMode);

    CompletableFuture<Void> commitBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, TxDescriptor txDescriptor);

    CompletableFuture<Void> abortBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, TxDescriptor txDescriptor);

    CompletableFuture<ByteBuffer> getBlob(AuthenticationToken token, String domainName, String volumeName, String blobName, int length, ObjectOffset offset);

    CompletableFuture<Void> updateMetadata(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                           TxDescriptor txDescriptor, Map<String, String> metadata);

    CompletableFuture<Void> updateBlob(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                       TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast);

    CompletableFuture<Void> updateBlobOnce(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                           int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata);

    CompletableFuture<Void> deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName);

    CompletableFuture<VolumeStatus> volumeStatus(AuthenticationToken token, String domainName, String volumeName);
}
