package com.formationds.xdi;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

import org.apache.thrift.TException;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */

public class FakeAsyncAm implements AsyncAm {
    private static final BlobDescriptor BLOB_DESCRIPTOR = new BlobDescriptor("foo", 4096, new HashMap<String, String>());
    private static final byte[] BYTES = new byte[4096];

    static {
        for (int i = 0; i < 4096; i++) {
            BYTES[i] = 42;
        }
    }

    @Override
    public void start() throws Exception {

    }

    @Override
    public CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<VolumeContents> volumeContents(String domainName,
                                                                  String volumeName,
                                                                  int count,
                                                                  long offset,
                                                                  String pattern,
                                                                  PatternSemantics patternSemantics,
                                                                  String delimiter,
                                                                  BlobListOrder order,
                                                                  boolean descending)
    {
        return CompletableFuture.completedFuture(new VolumeContents(new ArrayList<>(), new ArrayList<>()));
    }

    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        return CompletableFuture.completedFuture(BLOB_DESCRIPTOR);
    }

    @Override
    public CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName) {
        return CompletableFuture.completedFuture(BLOB_DESCRIPTOR);
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return CompletableFuture.completedFuture(new BlobWithMetadata(ByteBuffer.wrap(BYTES), BLOB_DESCRIPTOR));
    }

    @Override
    public CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode) {
        return CompletableFuture.completedFuture(new TxDescriptor(0));
    }

    @Override
    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return CompletableFuture.completedFuture(ByteBuffer.wrap(BYTES));
    }

    @Override
    public CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, Map<String, String> metadata) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName, int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        return CompletableFuture.completedFuture(new VolumeStatus(0, 0));
    }

    @Override
    public CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata) {
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName) {
        return CompletableFuture.completedFuture(new HashMap<>());
    }
}
