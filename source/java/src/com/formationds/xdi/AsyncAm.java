package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.security.DumbAuthorizer;
import com.formationds.util.BiConsumerWithException;
import com.formationds.util.ConsumerWithException;
import com.formationds.util.FunctionWithExceptions;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.s3.S3Endpoint;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TSimpleServer;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TServerSocket;

import java.nio.ByteBuffer;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertArrayEquals;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public class AsyncAm {
    public static final int PORT = 9876;
    public static final int BLOCKSIZE = 4096;
    private static final Logger LOG = Logger.getLogger(AsyncAm.class);
    private final AsyncAmResponseListener responseListener;
    private AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
    private AsyncAmServiceRequest.Iface oneWayAm;
    private Authorizer authorizer;
    private AsyncRequestStatistics statistics;

    public AsyncAm(AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool, AsyncAmServiceRequest.Iface oneWayAm, Authorizer authorizer) throws Exception {
        this.amPool = amPool;
        this.oneWayAm = oneWayAm;
        this.authorizer = authorizer;
        statistics = new AsyncRequestStatistics();
        responseListener = new AsyncAmResponseListener(2, TimeUnit.SECONDS);
        AsyncAmServiceResponse.Processor<AsyncAmResponseListener> processor = new AsyncAmServiceResponse.Processor<>(responseListener);

//        TNonblockingServer server = new TNonblockingServer(new TNonblockingServer.Args(new TNonblockingServerSocket(PORT))
//                .protocolFactory(new TBinaryProtocol.Factory())
//                .transportFactory(new TFramedTransport.Factory())
//                .processor(processor));
//
        TSimpleServer server = new TSimpleServer(new TServer.Args(new TServerSocket(PORT))
                .protocolFactory(new TBinaryProtocol.Factory())
                .transportFactory(new TFramedTransport.Factory())
                .processor(processor));

        new Thread(() -> server.serve(), "AM async listener thread").start();
        LOG.info("Started async AM listener on port " + PORT);
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 3) {
            System.out.println("Usage: AsyncAm hostName volumeName objectName");
            System.exit(-1);
        }
        String host = args[0];
        String volumeName = args[1];
        String objectName = args[2];
        XdiClientFactory factory = new XdiClientFactory();
        AsyncAmServiceRequest.Iface am = factory.remoteOnewayAm(host, 8899);
        byte[] bytes = new byte[BLOCKSIZE];
        SecureRandom secureRandom = new SecureRandom();
        secureRandom.nextBytes(bytes);
        am.updateBlobOnce(new RequestId(UUID.randomUUID().toString()), S3Endpoint.FDS_S3, volumeName, objectName, 1, ByteBuffer.wrap(bytes), BLOCKSIZE, new ObjectOffset(0), new HashMap<>());
    }

    public static void _main(String[] args) throws Exception {
        SecureRandom secureRandom = new SecureRandom();
        int length = 4096;
        byte[] bytes = new byte[length];
        secureRandom.nextBytes(bytes);
        String volumeName = UUID.randomUUID().toString();
        String blobName = UUID.randomUUID().toString();

        XdiClientFactory factory = new XdiClientFactory();
        VolumeSettings volumeSettings = new VolumeSettings();
        volumeSettings.setVolumeType(VolumeType.OBJECT);
        volumeSettings.setMaxObjectSizeInBytes(1024 * 1024 * 2);

        factory.remoteOmService("localhost", 9090).createVolume(S3Endpoint.FDS_S3, volumeName, volumeSettings, 0);
        Thread.sleep(2000);
        System.out.println("Done creating volume");
        AsyncAm asyncAm = new AsyncAm(factory.makeAmAsyncPool("localhost", 9988), factory.remoteOnewayAm("localhost", 8899), new DumbAuthorizer());
        Thread.sleep(500);

        CompletableFuture<Void> updateCf = asyncAm.updateBlobOnce(AuthenticationToken.ANONYMOUS, S3Endpoint.FDS_S3, volumeName, blobName, 1, ByteBuffer.wrap(bytes), length, new ObjectOffset(0), new HashMap<>());
        updateCf.get();
        CompletableFuture<ByteBuffer> readCf = asyncAm.getBlob(AuthenticationToken.ANONYMOUS, S3Endpoint.FDS_S3, volumeName, blobName, length, new ObjectOffset(0));
        ByteBuffer byteBuffer = readCf.get();
        byte[] read = new byte[length];
        byteBuffer.get(read);
        assertArrayEquals(bytes, read);
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

        RequestId requestId = new RequestId(UUID.randomUUID().toString());
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
        return schedule(token, "getBlob", volumeName, (am, cf) -> {
            am.getBlob(domainName, volumeName, blobName, length, offset, makeThriftCallbacks(cf, v -> v.getResult()));
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
//        return schedule(token, "updateBlobOnce", volumeName, (am, cf) -> {
//            am.updateBlobOnce(domainName, volumeName, blobName, blobMode, bytes, length, new ObjectOffset(offset), metadata, makeThriftCallbacks(cf, v -> null));
//        });
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
