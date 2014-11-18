package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.security.FastUUID;
import com.formationds.util.BiConsumerWithException;
import com.formationds.util.ConsumerWithException;
import com.formationds.util.FunctionWithExceptions;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.async.CompletableFutureUtility;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TSimpleServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TServerSocket;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAm {
    public int port = 9876;
    private static final Logger LOG = Logger.getLogger(AsyncAm.class);
    private final AsyncAmResponseListener responseListener;
    private AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
    private AsyncAmServiceRequest.Iface oneWayAm;
    private Authorizer authorizer;
    private AsyncRequestStatistics statistics;

    public AsyncAm(AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool, AsyncAmServiceRequest.Iface oneWayAm, Authorizer authorizer, int instanceId) throws Exception {
        this.amPool = amPool;
        this.oneWayAm = oneWayAm;
        this.authorizer = authorizer;
        statistics = new AsyncRequestStatistics();
        responseListener = new AsyncAmResponseListener(2, TimeUnit.SECONDS);
        port += instanceId;
    }

    public void start() throws Exception {
        AsyncAmServiceResponse.Processor<AsyncAmResponseListener> processor = new AsyncAmServiceResponse.Processor<>(responseListener);

        TSimpleServer server = new TSimpleServer(new TServer.Args(new TServerSocket(port))
                .protocolFactory(new TBinaryProtocol.Factory())
                .transportFactory(new TFramedTransport.Factory())
                .processor(processor));

        new Thread(() -> server.serve(), "AM async listener thread").start();
        LOG.info("Started async AM listener on port " + port);
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

    private <T> CompletableFuture<T> schedule(AuthenticationToken token, String actionName, String volumeName, BiConsumerWithException<AmService.AsyncIface, CompletableFuture<T>> consumer) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }

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

    private <T> CompletableFuture<T> scheduleAsync(AuthenticationToken token, String domainName, String volumeName, ConsumerWithException<RequestId> consumer) {
        if (!authorizer.hasAccess(token, volumeName)) {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }


        RequestId requestId = new RequestId(FastUUID.randomUUID().toString());
        try {
            CompletableFuture<T> cf = responseListener.expect(requestId);
            consumer.accept(requestId);
            return cf;
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> attachVolume(AuthenticationToken token, String domainName, String volumeName) throws TException {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.attachVolume(rid, domainName, volumeName);
        });
    }

    public CompletableFuture<List<BlobDescriptor>> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.volumeContents(rid, domainName, volumeName, count, offset);
        });
    }

    public CompletableFuture<BlobDescriptor> statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.statBlob(rid, domainName, volumeName, blobName);
        });
    }

    // This one
    public CompletableFuture<TxDescriptor> startBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, int blobMode) {
        return scheduleAsync(token, domainName, volumeName, rid ->
                oneWayAm.startBlobTx(rid, domainName, volumeName, blobName, blobMode));
    }

    public CompletableFuture<Void> commitBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.commitBlobTx(rid, domainName, volumeName, blobName, txDescriptor);
        });
    }

    public CompletableFuture<Void> abortBlobTx(AuthenticationToken token, String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.abortBlobTx(rid, domainName, volumeName, blobName, txDescriptor);
        });
    }

    public CompletableFuture<ByteBuffer> getBlob(AuthenticationToken token, String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.getBlob(rid, domainName, volumeName, blobName, length, offset);
        });
    }

    public CompletableFuture<Void> updateMetadata(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                                  TxDescriptor txDescriptor, Map<String, String> metadata) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.updateMetadata(rid, domainName, volumeName, blobName, txDescriptor, metadata);
        });
    }

    public CompletableFuture<Void> updateBlob(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                              TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.updateBlob(rid, domainName, volumeName, blobName, txDescriptor, bytes, length, new ObjectOffset(objectOffset), isLast);
        });
    }

    public CompletableFuture<Void> updateBlobOnce(AuthenticationToken token, String domainName, String volumeName, String blobName,
                                                  int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        return scheduleAsync(token, domainName, volumeName, rid ->
                oneWayAm.updateBlobOnce(rid, domainName, volumeName, blobName, blobMode, bytes, length, offset, metadata));
    }

    public CompletableFuture<Void> deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) {
        return schedule(token, "deleteBlob", volumeName, (am, cf) -> {
            am.deleteBlob(domainName, volumeName, blobName, makeThriftCallbacks(cf, v -> null));
        });
    }

    public CompletableFuture<VolumeStatus> volumeStatus(AuthenticationToken token, String domainName, String volumeName) {
        return scheduleAsync(token, domainName, volumeName, rid -> {
            oneWayAm.volumeStatus(rid, domainName, volumeName);
        });
    }
}
