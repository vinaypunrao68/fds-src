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

public interface AsyncAm {
    void start() throws Exception;

    CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException;

    CompletableFuture<List<BlobDescriptor>> volumeContents(String domainName, String volumeName, int count, long offset);

    CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName);

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
}
