package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.async.AsyncMessageDigest;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.blob.Mode;
import org.apache.commons.codec.binary.Hex;
import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.eclipse.jetty.io.ByteBufferPool;
import org.joda.time.DateTime;

import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class XdiAsyncImpl implements XdiAsync {

    public static class PutParameters {
        public final String domain;
        public final String volume;
        public final String blob;
        public final Function<ByteBuffer, CompletableFuture<Void>> reader;
        public final int objectSize;
        public final AsyncMessageDigest digest;
        public final Map<String, String> metadata;

        private PutParameters(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader, int objectSize, AsyncMessageDigest digest, Map<String, String> metadata) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            this.reader = reader;
            this.objectSize = objectSize;
            this.digest = digest;
            this.metadata = metadata;
        }

        private CompletableFuture<Map<String, String>> getFinalizedMetadata() {
            HashMap<String, String> md = new HashMap<>(metadata);
            md.computeIfAbsent(Xdi.LAST_MODIFIED, lm -> Long.toString(DateTime.now().getMillis()));
            return digest.get()
                    .thenApply(bytes -> {
                        md.put("etag", Hex.encodeHexString(bytes));
                        return md;
                    });
        }
    }

    private final AuthenticationToken token;
    private AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
    private Authorizer authorizer;
    private AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool;
    private ByteBufferPool bufferPool;
    private ConfigurationApi configurationApi;
    private AsyncRequestStatistics statistics;

    public XdiAsyncImpl(Authorizer authorizer,
                    AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool,
                    AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool,
                    ByteBufferPool bufferPool,
                    AuthenticationToken token,
                    ConfigurationApi configurationApi) {
        this.authorizer = authorizer;
        this.csPool = csPool;
        this.amPool = amPool;
        this.bufferPool = bufferPool;
        this.token = token;
        this.configurationApi = configurationApi;
        statistics = new AsyncRequestStatistics();
    }

    private static <T, R> AsyncMethodCallback<T> makeThriftCallbacks(CompletableFuture<R> future, FunctionWithExceptions<T, R> extractor) {
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

    public XdiAsync withStats(AsyncRequestStatistics statistics) {
        this.statistics = statistics;
        return this;
    }

    public CompletableFuture<BlobInfo> getBlobInfo(String domain, String volume, String blob) {
        CompletableFuture<BlobDescriptor> blobDescriptorFuture = statBlob(domain, volume, blob);
        CompletableFuture<VolumeDescriptor> volumeDescriptorFuture = statVolume(domain, volume);
        CompletableFuture<ByteBuffer> getObject0Future = volumeDescriptorFuture.thenCompose(vd -> getBlob(domain, volume, blob, 0, vd.getPolicy().maxObjectSizeInBytes));
        return blobDescriptorFuture.thenCompose(bd -> volumeDescriptorFuture.thenCombine(getObject0Future, (vd, o0) -> new BlobInfo(domain, volume, blob, bd, vd, o0)));
    }

    public CompletableFuture<PutResult> putBlobFromStream(String domain, String volume, String blob,  Map<String, String> metadata, InputStream stream) {
        // TODO: refactor digest into a more reusable form
        return putBlobFromSupplier(domain, volume, blob, metadata, bytes -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            CompletableFuture.runAsync(() -> {
                try {
                    int initialPosition = bytes.position();
                    ByteBufferUtility.readIntoByteBuffer(bytes, stream);
                    ByteBufferUtility.flipRelative(bytes, initialPosition);
                    cf.complete(null);
                } catch (Exception e) {
                    cf.completeExceptionally(e);
                }
            });
            return cf;
        });
    }

    public CompletableFuture<Void> getBlobToStream(BlobInfo blob, OutputStream str) {
        return getBlobToConsumer(blob, bytes -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            CompletableFuture.runAsync(() -> {
                try {
                    ByteBufferUtility.writeFromByteBuffer(bytes, str);
                    cf.complete(null);
                } catch (Exception e) {
                    cf.completeExceptionally(e);
                }
            });

            return cf;
        });
    }

    public CompletableFuture<Void> getBlobToConsumer(BlobInfo blobInfo, Function<ByteBuffer, CompletableFuture<Void>> processor) {
        //CompletableFuture<BlobDescriptor> blobDescriptorFuture = statBlob(domain, volume, blob);

        int objectSize = blobInfo.volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();

        // TODO: do we need to worry about limiting reads?
        ArrayList<CompletableFuture<ByteBuffer>> readFutures = new ArrayList<>();
        for (FdsObjectFrame frame : FdsObjectFrame.frames(0, blobInfo.blobDescriptor.byteCount, objectSize)) {
            if (frame.objectOffset == 0)
                readFutures.add(CompletableFuture.completedFuture(blobInfo.object0));
            else
                readFutures.add(getBlob(blobInfo.domain, blobInfo.volume, blobInfo.blob, frame.objectOffset, frame.internalLength));
        }

        CompletableFuture<Void> result = CompletableFuture.completedFuture(null);
        for (CompletableFuture<ByteBuffer> readFuture : readFutures) {
            result = result.thenCompose(r -> readFuture.thenCompose(buf -> statistics.time("writeOutputTime", processor.apply(buf))));
        }
        return result;
    }

    public CompletableFuture<PutResult> putBlobFromSupplier(String domain, String volume, String blob, Map<String, String> metadata, Function<ByteBuffer, CompletableFuture<Void>> reader) {
        try {
            AsyncMessageDigest asmd = new AsyncMessageDigest(MessageDigest.getInstance("MD5"));
            return statVolume(domain, volume)
                    .thenCompose(volumeDescriptor -> firstPut(new PutParameters(domain, volume, blob, reader, volumeDescriptor.getPolicy().getMaxObjectSizeInBytes(), asmd, metadata), 0))
                    .thenCompose(_completion -> asmd.get())
                    .thenApply(digestBytes -> new PutResult(digestBytes));
        } catch (NoSuchAlgorithmException ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    private ByteBuffer condenseBuffer(ByteBuffer buffer) {
        if (buffer.remaining() < buffer.capacity() / 2) {
            ByteBuffer targetBuffer = ByteBuffer.allocate(buffer.remaining()); //bufferPool.acquire(buffer.remaining(), false);
            targetBuffer.limit(targetBuffer.capacity());
            targetBuffer.put(buffer);
            targetBuffer.flip();
            bufferPool.release(buffer);
            return targetBuffer;
        }
        return buffer;
    }

    private CompletableFuture<Void> firstPut(PutParameters putParameters, long objectOffset) {
        ByteBuffer readBuf = bufferPool.acquire(putParameters.objectSize, false);
        readBuf.limit(putParameters.objectSize);
        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);

        return readCompleteFuture.thenCompose(_null -> {
            final ByteBuffer condensedBuffer = condenseBuffer(readBuf);
            putParameters.digest.update(condensedBuffer);
            if (condensedBuffer.remaining() < putParameters.objectSize) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                        updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), condensedBuffer, condensedBuffer.remaining(), objectOffset, md)
                                .whenComplete((_null2, ex) -> bufferPool.release(condensedBuffer)));
            } else {
                return secondPut(putParameters, objectOffset + 1, condensedBuffer);
            }
        });
    }

    private CompletableFuture<Void> secondPut(PutParameters putParameters, long objectOffset, ByteBuffer first) {
        ByteBuffer readBuf = bufferPool.acquire(putParameters.objectSize, false);
        readBuf.limit(putParameters.objectSize);

        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);
        return readCompleteFuture.thenCompose(_null -> {
            final ByteBuffer condensedBuffer = condenseBuffer(readBuf);
            if (condensedBuffer.remaining() == 0) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                        updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), first, putParameters.objectSize, objectOffset, md).whenComplete((_null2, ex) -> bufferPool.release(condensedBuffer)));
            } else {
                CompletableFuture<Void> digestFuture = putParameters.digest.update(condensedBuffer);
                return createTx(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue()).thenComposeAsync(tx -> {
                    CompletableFuture<Void> firstResult = tx.update(first, objectOffset - 1, putParameters.objectSize)
                            .thenCompose(_null1 -> digestFuture)
                            .whenComplete((_r1, _ex1) -> bufferPool.release(first));
                    CompletableFuture<Void> secondResult = tx.update(condensedBuffer, objectOffset, condensedBuffer.remaining())
                            .thenCompose(_null1 -> digestFuture)
                            .whenComplete((_r1, _ex1) -> bufferPool.release(condensedBuffer));

                    CompletableFuture<Void> rest = putSequence(putParameters, tx, objectOffset + 1)
                            .thenCompose(_sequenceCompletion -> putParameters.getFinalizedMetadata())
                            .thenCompose(metadata -> tx.updateMetadata(metadata));
                    return completeTransaction(tx, CompletableFuture.allOf(firstResult, secondResult, rest));
                });
            }
        });
    }

    private CompletableFuture<Void> completeTransaction(TransactionHandle tx, CompletableFuture<Void> priorFuture) {
        return thenComposeExceptionally(priorFuture.thenCompose(_null -> tx.commit()), _null -> tx.abort());
    }

    private CompletableFuture<Void> putSequence(PutParameters putParameters, TransactionHandle tx, long objectOffset) {
        ByteBuffer readBuf = bufferPool.acquire(putParameters.objectSize, false);
        readBuf.limit(putParameters.objectSize);
        CompletableFuture<ByteBuffer> readCompleteFuture = putParameters.reader.apply(readBuf).thenApply(_null -> condenseBuffer(readBuf));

        return readCompleteFuture.thenCompose(condensedBuffer -> {
            CompletableFuture<Void> digestFuture = putParameters.digest.update(condensedBuffer);
            CompletableFuture<Void> writeFuture = null;
            CompletableFuture<Void> readNextFuture = null;
            if (condensedBuffer.remaining() == 0) {
                bufferPool.release(condensedBuffer);
                writeFuture = CompletableFuture.<Void>completedFuture(null);
            } else {
                writeFuture = tx.update(condensedBuffer, objectOffset, condensedBuffer.remaining())
                        .thenCompose(_null1 -> digestFuture)
                        .whenComplete((r, ex) -> bufferPool.release(condensedBuffer));
            }

            if (condensedBuffer.remaining() < putParameters.objectSize)
                readNextFuture = CompletableFuture.<Void>completedFuture(null);
            else
                readNextFuture = digestFuture.thenCompose(_digestCompletion -> putSequence(putParameters, tx, objectOffset + 1));

            return CompletableFuture.allOf(readNextFuture, writeFuture);
        });
    }

    public <T> CompletableFuture<T> amUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<AmService.AsyncIface>, CompletableFuture<T>> operation) {
        if(authorizer.hasAccess(token, volume)) {
            CompletableFuture<Void> poolWaitTime = statistics.time("amPoolWaitTime", new CompletableFuture<>());
            return amPool.use(am -> {
                poolWaitTime.complete(null);
                statistics.note("am connection pool sample: " + amPool.getUsedCount());
                return operation.apply(am);
            });
        } else {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }
    }

    public <T> CompletableFuture<T> csUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<ConfigurationService.AsyncIface>, CompletableFuture<T>> operation) {
        if (authorizer.hasAccess(token, volume)) {
            CompletableFuture<Void> poolWaitTime = statistics.time("csPoolWaitTime", new CompletableFuture<>());
            return csPool.use(cs -> {
                poolWaitTime.complete(null);
                return operation.apply(cs);
            });
        } else {
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
        }
    }

    public CompletableFuture<VolumeDescriptor> statVolume(String domain, String volume) {
        CompletableFuture<VolumeDescriptor> cf = new CompletableFuture<>();
        try {
            VolumeDescriptor volumeDescriptor = configurationApi.statVolume(domain, volume);
            cf.complete(volumeDescriptor);
        } catch (TException e) {
            cf.completeExceptionally(e);
        }

        return cf;
    }

    public CompletableFuture<BlobDescriptor> statBlob(String domain, String volume, String blob) {
        return amUseVolume(volume, am -> {
            CompletableFuture<BlobDescriptor> result = statistics.time("statBlob", new CompletableFuture<>());
            am.getClient().statBlob(domain, volume, blob, makeThriftCallbacks(result, r -> r.getResult()));
            return result;
        });
    }

    public CompletableFuture<ByteBuffer> getBlob(String domain, String volume, String blob, long offset, int length) {
        return amUseVolume(volume, am -> {
            CompletableFuture<ByteBuffer> result = statistics.time("getBlob", new CompletableFuture<>());
            am.getClient().getBlob(domain, volume, blob, length, new ObjectOffset(offset), makeThriftCallbacks(result, r -> r.getResult()));
            return result;
        });
    }

    public CompletableFuture<Void> updateBlobOnce(String domain, String volume, String blob, int blobMode, ByteBuffer data, int length, long offset, Map<String, String> metadata) {
        return amUseVolume(volume, am -> {
            CompletableFuture<Void> result = statistics.time("updateBlobOnce", new CompletableFuture<>());
            am.getClient().updateBlobOnce(domain, volume, blob, blobMode, data, length, new ObjectOffset(offset), metadata, makeThriftCallbacks(result, r -> null));
            return result;
        });
    }

    public CompletableFuture<TransactionHandle> createTx(String domain, String volume, String blob, int mode) {
        return amUseVolume(volume, am -> {
            CompletableFuture<TransactionHandle> result = statistics.time("createTx", new CompletableFuture<>());
            am.getClient().startBlobTx(domain, volume, blob, mode, makeThriftCallbacks(result, r -> new TransactionHandleImpl(domain, volume, blob, r.getResult())));
            return result;
        });
    }

    private <T> CompletableFuture<T> thenComposeExceptionally(CompletableFuture<T> future, Function<Throwable, CompletableFuture<Void>> handler) {
        CompletableFuture<T> result = new CompletableFuture<>();

        future.whenComplete((r, ex) -> {
            if (ex == null)
                result.complete(r);
            else
                handler.apply(ex).thenApply(_null -> result.completeExceptionally(ex));
        });

        return result;
    }

    public static class Factory {
        private final Authorizer authorizer;
        private final AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool;
        private final AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
        private final ByteBufferPool bufferPool;
        private ConfigurationApi configurationApi;

        public Factory(Authorizer authorizer,
                       AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool,
                       AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool,
                       ByteBufferPool bufferPool,
                       ConfigurationApi configurationApi) {
            this.authorizer = authorizer;
            this.csPool = csPool;
            this.amPool = amPool;
            this.bufferPool = bufferPool;
            this.configurationApi = configurationApi;
        }

        public XdiAsync createAuthenticated(AuthenticationToken token) {
            return new XdiAsyncImpl(authorizer, csPool, amPool, bufferPool, token, configurationApi);
        }
    }

    public class TransactionHandleImpl implements TransactionHandle {
        private final String domain;
        private final String volume;
        private final String blob;
        private final TxDescriptor descriptor;

        private TransactionHandleImpl(String domain, String volume, String blob, TxDescriptor descriptor) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            this.descriptor = descriptor;
        }

        public CompletableFuture<Void> commit() {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = statistics.time("commitBlobTx", new CompletableFuture<>());
                am.getClient().commitBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }

        public CompletableFuture<Void> abort() {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = statistics.time("abortBlobTx", new CompletableFuture<>());
                am.getClient().abortBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }

        public CompletableFuture<Void> update(ByteBuffer buffer, long objectOffset, int length) {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = statistics.time("updateBlob", new CompletableFuture<>());
                am.getClient().updateBlob(domain, volume, blob, descriptor, buffer, length, new ObjectOffset(objectOffset), false, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }

        public CompletableFuture<Void> updateMetadata(Map<String, String> meta) {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = statistics.time("updateMetadata", new CompletableFuture<Void>());
                am.getClient().updateMetadata(domain, volume, blob, descriptor, meta, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }
    }
}
