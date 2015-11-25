package com.formationds.xdi;

import com.formationds.apis.AsyncXdiServiceResponse;
import com.formationds.apis.RequestId;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.ErrorCode;
import com.formationds.protocol.VolumeAccessMode;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.joda.time.Duration;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAmResponseListener implements AsyncXdiServiceResponse.Iface {
    private static final Logger LOG = Logger.getLogger(AsyncAmResponseListener.class);
    private Cache<String, CompletableFuture> pending;
    private ExecutorService executor;

    public AsyncAmResponseListener(Duration timeout) {
        executor = Executors.newCachedThreadPool();
        this.pending = CacheBuilder.newBuilder()
                .removalListener(notification -> {
                    if (notification.getCause().equals(RemovalCause.EXPIRED)) {
                        CompletableFuture cf = (CompletableFuture) notification.getValue();
                        if (!cf.isDone()) {
                            executor.execute(
                                    () -> cf.completeExceptionally(new ApiException("Request timed out", ErrorCode.TIMEOUT)));
                        }
                    }
                })
                .expireAfterWrite(timeout.getMillis(), TimeUnit.MILLISECONDS)
                .build();
    }

    public void start() {
        new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    break;
                }
                pending.cleanUp();
            }
        }, "Async AM response listener clean-up").start();
        LOG.debug("Started async AM stale request scavenger");
    }

    public <T> CompletableFuture<T> expect(RequestId requestId) {
        CompletableFuture<T> cf = new CompletableFuture<T>();
        pending.put(requestId.getId(), cf);
        return cf;
    }

    @Override
    public void handshakeComplete(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void attachVolumeResponse(RequestId requestId, VolumeAccessMode mode) throws TException {
        complete(requestId, null);
    }

    @Override
    public void volumeContents(RequestId requestId, List<BlobDescriptor> blobDescriptors, List<String> skippedPrefixes) throws TException {
        complete(requestId, new VolumeContents(blobDescriptors, skippedPrefixes));
    }

    @Override
    public void setVolumeMetadataResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void getVolumeMetadataResponse(RequestId requestId, Map<String,String> metadata) throws TException {
        complete(requestId, metadata);
    }

    @Override
    public void statBlobResponse(RequestId requestId, BlobDescriptor blobDescriptor) throws TException {
        complete(requestId, blobDescriptor);
    }

    @Override
    public void startBlobTxResponse(RequestId requestId, TxDescriptor txDescriptor) throws TException {
        complete(requestId, txDescriptor);
    }

    @Override
    public void commitBlobTxResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void abortBlobTxResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void getBlobResponse(RequestId requestId, ByteBuffer byteBuffer) throws TException {
        complete(requestId, byteBuffer);
    }

    @Override
    public void getBlobWithMetaResponse(RequestId requestId, ByteBuffer byteBuffer, BlobDescriptor blobDescriptor) throws TException {
        complete(requestId, new BlobWithMetadata(byteBuffer, blobDescriptor));
    }

    @Override
    public void renameBlobResponse(RequestId requestId, BlobDescriptor blobDescriptor) throws TException {
        complete(requestId, blobDescriptor);
    }

    @Override
    public void updateMetadataResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void updateBlobResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void updateBlobOnceResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void deleteBlobResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void volumeStatus(RequestId requestId, VolumeStatus volumeStatus) throws TException {
        complete(requestId, volumeStatus);
    }

    @Override
    public void completeExceptionally(RequestId requestId, ErrorCode errorCode, String s) throws TException {
        CompletableFuture cf = pending.getIfPresent(requestId.getId());
        if (cf == null) {
            LOG.error("RequestId " + requestId.getId() + " had no pending requests");
            return;
        }
        pending.invalidate(requestId.getId());
        executor.execute(() -> cf.completeExceptionally(new ApiException(s, errorCode)));
    }

    private <T> void complete(RequestId requestId, T tee) {
        CompletableFuture cf = pending.getIfPresent(requestId.getId());
        if (cf == null) {
            LOG.error("RequestId " + requestId.getId() + " had no pending requests");
            return;
        }
        executor.execute(() -> cf.complete(tee));
        pending.invalidate(requestId.getId());
    }

    public void expireOldEntries() {
        pending.cleanUp();
    }
}
