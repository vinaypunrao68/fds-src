package com.formationds.xdi;

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

public interface AsyncAm {
    void start() throws Exception;

    CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException;

    CompletableFuture<List<BlobDescriptor>> volumeContents(String domainName,
                                                           String volumeName,
                                                           int count,
                                                           long offset,
                                                           String pattern,
                                                           PatternSemantics patternSemantics,
                                                           String delimiter,
                                                           BlobListOrder order,
                                                           boolean descending);

    CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName);

    CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName);

    CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset);

    // This one
    CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode);

    CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor);

    CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor);

    CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset);

    CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName,
                                           TxDescriptor txDescriptor, Map<String, String> metadata);

    CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName,
                                       TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast);

    CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName,
                                           int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata);

    CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName);

    CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName);

    CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata);

    CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName);
}
