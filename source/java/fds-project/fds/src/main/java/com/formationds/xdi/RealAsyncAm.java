package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.protocol.VolumeAccessMode;
import com.formationds.security.FastUUID;
import com.formationds.util.ConsumerWithException;
import com.formationds.util.Retry;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.joda.time.Duration;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public class RealAsyncAm implements AsyncAm {
    private static final Logger LOG = Logger.getLogger(RealAsyncAm.class);
    private final AsyncAmResponseListener responseListener;
    private AsyncXdiServiceRequest.Iface oneWayAm;
    private String amHost;
    private int amPort;
    private int responsePort;
    private AsyncRequestStatistics statistics;

    public RealAsyncAm(String amHost, int amPort, int responseServerPort) throws Exception {
        this(amHost, amPort, responseServerPort, 30, TimeUnit.SECONDS);
    }

    public RealAsyncAm(String amHost, int amPort, int responsePort, int timeoutDuration, TimeUnit timeoutDurationUnit) throws Exception {
        this.amHost = amHost;
        this.amPort = amPort;
        this.responsePort = responsePort;
        statistics = new AsyncRequestStatistics();
        responseListener = new AsyncAmResponseListener(timeoutDuration, timeoutDurationUnit);
    }

    @Override
    public void start() throws Exception {
        try {
            AsyncXdiServiceResponse.Processor<AsyncAmResponseListener> processor = new AsyncXdiServiceResponse.Processor<>(responseListener);

            TNonblockingServer server = new TNonblockingServer(
                    new TNonblockingServer.Args(new TNonblockingServerSocket(responsePort))
                            .processor(processor));

            new Thread(() -> server.serve(), "AM async listener thread").start();
            LOG.debug("Started async AM listener on port " + responsePort);

            responseListener.start();

            ThriftClientFactory<AsyncXdiServiceRequest.Iface> factory = new ThriftClientFactory.Builder<>(AsyncXdiServiceRequest.Iface.class)
                    .withHostPort(amHost, amPort)
                    .withPoolConfig(100, 0, Integer.MAX_VALUE)
                    .withClientFactory(bp -> {
                        AsyncXdiServiceRequest.Client client = new AsyncXdiServiceRequest.Client(bp);
                        try {
                            Retry<Void, Void> retry = new Retry<>(
                                    (x, numTries) -> {
                                        RequestId requestId = new RequestId(UUID.randomUUID().toString());
                                        CompletableFuture<Void> cf = responseListener.expect(requestId);
                                        client.handshakeStart(requestId, responsePort);
                                        cf.get();
                                        return null;
                                    },
                                    120,
                                    Duration.standardSeconds(1),
                                    "async handshake with bare_am"
                            );
                            retry.apply(null);
                        } catch (Exception e) {
                            throw new RuntimeException(e);
                        }
                        return client;
                    })
                    .build();

            oneWayAm = factory.getClient();

        } catch (Exception e) {
            LOG.error("Error starting async AM", e);
            throw e;
        }
    }

    private <T> CompletableFuture<T> scheduleAsync(ConsumerWithException<RequestId> consumer) {
        RequestId requestId = new RequestId(FastUUID.randomUUID().toString());
        try {
            CompletableFuture<T> cf = responseListener.expect(requestId);
            consumer.accept(requestId);
            return cf;
        } catch (Exception e) {
            return CompletableFutureUtility.exceptionFuture(e);
        }
    }

    public CompletableFuture<Void> handshake(int port) {
        return scheduleAsync(rid -> {
            oneWayAm.handshakeStart(rid, port);
        });
    }

    @Override
    public CompletableFuture<Void> attachVolume(String domainName, String volumeName) throws TException {
        return scheduleAsync(rid -> {
            // TODO (bszmyd)
            // Should probably expose this ability to attach with an explicit mode to the connectors
            // for now we just default to r/w with cache
            VolumeAccessMode mode = new VolumeAccessMode();
            oneWayAm.attachVolume(rid, domainName, volumeName, mode);
        });
    }

    @Override
    public CompletableFuture<List<BlobDescriptor>> volumeContents(String domainName, String volumeName, int count, long offset, String pattern, BlobListOrder order, boolean descending) {
        return scheduleAsync(rid -> {
            oneWayAm.volumeContents(rid, domainName, volumeName, count, offset, pattern, PatternSemantics.PCRE, order, descending, "/");
        });
    }

    @Override
    public CompletableFuture<BlobDescriptor> statBlob(String domainName, String volumeName, String blobName) {
        return scheduleAsync(rid -> {
            oneWayAm.statBlob(rid, domainName, volumeName, blobName);
        });
    }

    @Override
    public CompletableFuture<BlobDescriptor> renameBlob(String domainName, String volumeName, String sourceBlobName, String destinationBlobName) {
        return scheduleAsync(rid -> {
            oneWayAm.renameBlob(rid, domainName, volumeName, sourceBlobName, destinationBlobName);
        });
    }

    @Override
    public CompletableFuture<BlobWithMetadata> getBlobWithMeta(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return scheduleAsync(rid -> {
            oneWayAm.getBlobWithMeta(rid, domainName, volumeName, blobName, length, offset);
        });
    }

    // This one
    @Override
    public CompletableFuture<TxDescriptor> startBlobTx(String domainName, String volumeName, String blobName, int blobMode) {
        return scheduleAsync(rid ->
                oneWayAm.startBlobTx(rid, domainName, volumeName, blobName, blobMode));
    }

    @Override
    public CompletableFuture<Void> commitBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return scheduleAsync(rid -> {
            oneWayAm.commitBlobTx(rid, domainName, volumeName, blobName, txDescriptor);
        });
    }

    @Override
    public CompletableFuture<Void> abortBlobTx(String domainName, String volumeName, String blobName, TxDescriptor txDescriptor) {
        return scheduleAsync(rid -> {
            oneWayAm.abortBlobTx(rid, domainName, volumeName, blobName, txDescriptor);
        });
    }

    @Override
    public CompletableFuture<ByteBuffer> getBlob(String domainName, String volumeName, String blobName, int length, ObjectOffset offset) {
        return scheduleAsync(rid -> {
            oneWayAm.getBlob(rid, domainName, volumeName, blobName, length, offset);
        });
    }

    @Override
    public CompletableFuture<Void> updateMetadata(String domainName, String volumeName, String blobName,
                                                  TxDescriptor txDescriptor, Map<String, String> metadata) {
        return scheduleAsync(rid -> {
            oneWayAm.updateMetadata(rid, domainName, volumeName, blobName, txDescriptor, metadata);
        });
    }

    @Override
    public CompletableFuture<Void> updateBlob(String domainName, String volumeName, String blobName,
                                              TxDescriptor txDescriptor, ByteBuffer bytes, int length, ObjectOffset objectOffset, boolean isLast) {
        return scheduleAsync(rid -> {
            oneWayAm.updateBlob(rid, domainName, volumeName, blobName, txDescriptor, bytes, length, new ObjectOffset(objectOffset));
        });
    }

    @Override
    public CompletableFuture<Void> updateBlobOnce(String domainName, String volumeName, String blobName,
                                                  int blobMode, ByteBuffer bytes, int length, ObjectOffset offset, Map<String, String> metadata) {
        return scheduleAsync(rid ->
                oneWayAm.updateBlobOnce(rid, domainName, volumeName, blobName, blobMode, bytes, length, offset, metadata));
    }

    @Override
    public CompletableFuture<Void> deleteBlob(String domainName, String volumeName, String blobName) {
        CompletableFuture<TxDescriptor> startBlobCf = scheduleAsync(rid -> {
            oneWayAm.startBlobTx(rid, domainName, volumeName, blobName, 1);
        });

        CompletableFuture<Void> deleteCf = startBlobCf.thenCompose(tx -> scheduleAsync(rid -> {
            oneWayAm.deleteBlob(rid, domainName, volumeName, blobName, tx);
        }));

        return deleteCf.thenCompose(x -> startBlobCf.thenCompose(tx -> scheduleAsync(rid -> {
            oneWayAm.commitBlobTx(rid, domainName, volumeName, blobName, tx);
        })));
    }

    @Override
    public CompletableFuture<VolumeStatus> volumeStatus(String domainName, String volumeName) {
        return scheduleAsync(rid -> {
            oneWayAm.volumeStatus(rid, domainName, volumeName);
        });
    }

    @Override
    public CompletableFuture<Map<String, String>> getVolumeMetadata(String domainName, String volumeName) {
        return scheduleAsync(rid -> {
            oneWayAm.getVolumeMetadata(rid, domainName, volumeName);
        });
    }

    @Override
    public CompletableFuture<Void> setVolumeMetadata(String domainName, String volumeName, Map<String, String> metadata) {
        return scheduleAsync(rid -> {
            oneWayAm.setVolumeMetadata(rid, domainName, volumeName, metadata);
        });
    }
}
