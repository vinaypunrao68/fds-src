package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.util.BiConsumerWithException;
import com.formationds.util.FunctionWithExceptions;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;
import org.apache.thrift.async.AsyncMethodCallback;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAm {
    private AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
    private AsyncRequestStatistics statistics;

    public AsyncAm(AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool) {
        this.amPool = amPool;
        statistics = new AsyncRequestStatistics();
    }

    private <T, R> AsyncMethodCallback<T> makeThriftCallbacks(CompletableFuture<R> future, FunctionWithExceptions<T, R> extractor) {
        return new AsyncMethodCallback<T>() {
            @Override
            public void onComplete(T result) {
                try {
                    future.complete(extractor.apply(result));
                } catch (Exception ex) {
                    future.completeExceptionally(ex);
                }
            }

            @Override
            public void onError(Exception e) {
                future.completeExceptionally(e);
            }
        };
    }

    private <T> CompletableFuture<T> schedule(String actionName, String volumeName, BiConsumerWithException<AmService.AsyncIface, CompletableFuture<T>> consumer) {
        CompletableFuture<T> cf = statistics.time(actionName, new CompletableFuture<>());
        CompletableFuture<Void> poolWaitTime = statistics.time("amPoolWaitTime", new CompletableFuture<>());
        return amPool.use(am -> {
            poolWaitTime.complete(null);
            try {
                consumer.accept(am.getClient(), cf);
            } catch (Exception e) {
                cf.completeExceptionally(e);
            }
            return cf;
        });
    }

    public CompletableFuture<List<BlobDescriptor>> volumeContents(String domainName, String volumeName, int count, long offset) {
        return schedule("volumeContents", volumeName, (am, cf) -> {
            am.volumeContents(domainName, volumeName, count, offset, makeThriftCallbacks(cf, v -> v.getResult()));
        });
    }

    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        return schedule("statBlob", volumeName, (am, cf) -> {
            am.statBlob(domainName, volumeName, blobName, makeThriftCallbacks(cf, v -> v.getResult()));
        });
    }

    public CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode) {
        return amPool.use(am -> {
            CompletableFuture<TxDescriptor> cf = new CompletableFuture<>();
            am.getClient().startBlobTx(domainName, volumeName, blobName, blobMode, makeThriftCallbacks(cf, v -> v.getResult()));
            return cf;
        });
    }

    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().commitBlobTx(domainName, volumeName, blobName, txDescriptor, makeThriftCallbacks(cf, v -> null));
            return cf;
        });
    }

    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().abortBlobTx(domainName, volumeName, blobName, txDescriptor, makeThriftCallbacks(cf, v -> null));
            return cf;
        });
    }

    public CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return amPool.use(am -> {
            CompletableFuture<ByteBuffer> cf = new CompletableFuture<>();
            am.getClient().getBlob(domainName, volumeName, blobName, length, offset, makeThriftCallbacks(cf, v -> v.getResult()));
            return cf;
        });
    }

    public CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName,
                                                  TxDescriptor txDescriptor, Map<String, String> metadata) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().updateMetadata(domainName, volumeName, blobName, txDescriptor, metadata, makeThriftCallbacks(cf, v -> null));
            return cf;
        });
    }

    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName,
                                              TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().updateBlob(domainName, volumeName, blobName, txDescriptor, bytes, length, objectOffset, isLast, makeThriftCallbacks(cf, v -> null));
            return cf;
        });
    }

    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName,
                                                  int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().updateBlobOnce(domainName, volumeName, blobName, blobMode, bytes, length, offset, metadata, makeThriftCallbacks(cf, v -> null));
            return cf;
        });

    }

    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        return amPool.use(am -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            am.getClient().deleteBlob(domainName, volumeName, blobName, makeThriftCallbacks(cf, v -> null));
            return cf;
        });

    }

    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        return amPool.use(am -> {
            CompletableFuture<VolumeStatus> cf = new CompletableFuture<>();
            am.getClient().volumeStatus(domainName, volumeName, makeThriftCallbacks(cf, v -> v.getResult()));
            return cf;
        });
    }
}
