package com.formationds.xdi;

import com.formationds.apis.*;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAmResponseListener implements AsyncAmServiceResponse.Iface {
    private static final Logger LOG = Logger.getLogger(AsyncAmResponseListener.class);
    private Cache<String, CompletableFuture> pending;

    public AsyncAmResponseListener(long timeout, TimeUnit timeUnit) {
        this.pending = CacheBuilder.newBuilder()
                .removalListener(notification -> {
                    if (notification.getCause().equals(RemovalCause.EXPIRED)) {
                        CompletableFuture cf = (CompletableFuture) notification.getValue();
                        if (!cf.isDone()) {
                            cf.completeExceptionally(new ApiException("Request timed out", ErrorCode.INTERNAL_SERVER_ERROR));
                        }
                    }
                })
                .expireAfterWrite(timeout, timeUnit)
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
    }

    public <T> CompletableFuture<T> expect(RequestId requestId) {
        CompletableFuture<T> cf = new CompletableFuture<T>();
        pending.put(requestId.getId(), cf);
        return cf;
    }

    private <T> void complete(RequestId requestId, T tee) {
        CompletableFuture cf = pending.getIfPresent(requestId.getId());
        if (cf == null) {
            LOG.error("RequestId " + requestId.getId() + " had no pending requests");
            return;
        }
        cf.complete(tee);
        pending.invalidate(requestId.getId());
    }

    @Override
    public void attachVolumeResponse(RequestId requestId) throws TException {
        complete(requestId, null);
    }

    @Override
    public void volumeContents(RequestId requestId, List<BlobDescriptor> blobDescriptors) throws TException {
        complete(requestId, blobDescriptors);
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
        }
        cf.completeExceptionally(new ApiException(s, errorCode));
        pending.invalidate(requestId.getId());
    }

    public void expireOldEntries() {
        pending.cleanUp();
    }
}
