package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.async.AsyncMessageDigest;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.blob.Mode;
import com.formationds.web.toolkit.ServletInputWapper;
import com.formationds.web.toolkit.ServletOutputWrapper;
import org.apache.commons.codec.binary.Hex;
import org.apache.thrift.async.AsyncMethodCallback;
import org.eclipse.jetty.io.ByteBufferPool;
import org.joda.time.DateTime;

import javax.servlet.ServletInputStream;
import javax.servlet.ServletOutputStream;
import java.io.IOException;
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

public class XdiAsync {
    private AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
    private Authorizer authorizer;
    private AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool;
    private ByteBufferPool bufferPool;
    private final AuthenticationToken token;

    public XdiAsync(Authorizer authorizer,
                    AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool,
                    AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool,
                    ByteBufferPool bufferPool,
                    AuthenticationToken token) {
        this.authorizer = authorizer;
        this.csPool = csPool;
        this.amPool = amPool;
        this.bufferPool = bufferPool;
        this.token = token;
    }

    public CompletableFuture<PutResult> putBlobFromStream(String domain, String volume, String blob, InputStream stream) {
        // TODO: refactor digest into a more reusable form
        return putBlobFromSupplier(domain, volume, blob, bytes -> {
            CompletableFuture<Void> cf = new CompletableFuture<>();
            CompletableFuture.runAsync(() -> {
                try {
                    int initialPosition = bytes.position();
                    ByteBufferUtility.readIntoByteBuffer(bytes, stream);
                    ByteBufferUtility.flipRelative(bytes, initialPosition);
                    cf.complete(null);
                } catch (Exception e) {
                    cf.completeExceptionally(e);
                }});
            return cf;
        });
    }

    public CompletableFuture<Void> getBlobToStream(String domain, String volume, String blob, OutputStream str) {
        return getBlobToConsumer(domain, volume, blob, bytes -> {
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

    public CompletableFuture<Void> getBlobToConsumer(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> processor) {
        CompletableFuture<BlobDescriptor> blobDescriptorFuture = statBlob(domain, volume, blob);
        CompletableFuture<VolumeDescriptor> volumeDescriptorFuture = statVolume(domain, volume);

        return
            volumeDescriptorFuture.thenCompose(volumeDescriptor ->
                    blobDescriptorFuture.thenCompose(blobDescriptor -> {
                        int objectSize = volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();

                        // TODO: do we need to worry about limiting reads?
                        ArrayList<CompletableFuture<ByteBuffer>> readFutures = new ArrayList<>();
                        for (FdsObjectFrame frame : FdsObjectFrame.frames(0, blobDescriptor.byteCount, objectSize)) {
                            readFutures.add(getBlob(domain, volume, blob, frame.objectOffset, frame.internalLength));
                        }

                        CompletableFuture<Void> result = CompletableFuture.completedFuture(null);
                        for (CompletableFuture<ByteBuffer> readFuture : readFutures) {
                            result = result.thenCompose(r -> readFuture.thenCompose(processor));
                        }
                        return result;
                    }));
    }

    public CompletableFuture<PutResult> putBlobFromSupplier(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader) {
        try {
            AsyncMessageDigest asmd = new AsyncMessageDigest(MessageDigest.getInstance("MD5"));
            return statVolume(domain, volume)
                    .thenCompose(volumeDescriptor -> firstPut(new PutParameters(domain, volume, blob, reader, volumeDescriptor.getPolicy().getMaxObjectSizeInBytes(), asmd), 0))
                    .thenCompose(_completion -> asmd.get())
                    .thenApply(digestBytes -> new PutResult(digestBytes));
        } catch(NoSuchAlgorithmException ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    private CompletableFuture<Void> firstPut(PutParameters putParameters, long objectOffset) {
        ByteBuffer readBuf = bufferPool.acquire(putParameters.objectSize, false);
        readBuf.limit(putParameters.objectSize);
        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);

        return readCompleteFuture.thenCompose(_null -> {
            putParameters.digest.update(readBuf);
            if(readBuf.remaining() < putParameters.objectSize) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                    updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), readBuf, readBuf.remaining(), objectOffset, md).whenComplete((_null2, ex) -> bufferPool.release(readBuf)));
            } else {
                return secondPut(putParameters, objectOffset + 1, readBuf);
            }
        });
    }

    private CompletableFuture<Void> secondPut(PutParameters putParameters, long objectOffset, ByteBuffer first) {
        ByteBuffer readBuf = bufferPool.acquire(putParameters.objectSize, false);
        readBuf.limit(putParameters.objectSize);

        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);
        return readCompleteFuture.thenCompose(_null -> {
            if (readBuf.remaining() == 0) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                        updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), first, putParameters.objectSize, objectOffset, md).whenComplete((_null2, ex) -> bufferPool.release(readBuf)));
            } else {
                CompletableFuture<Void> digestFuture = putParameters.digest.update(readBuf);
                return createTx(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue()).thenComposeAsync(tx -> {
                    CompletableFuture<Void> firstResult = tx.update(first, objectOffset - 1, putParameters.objectSize)
                            .thenCompose(_null1 -> digestFuture)
                            .whenComplete((_r1, _ex1) -> bufferPool.release(first));
                    CompletableFuture<Void> secondResult = tx.update(readBuf, objectOffset, readBuf.remaining())
                            .thenCompose(_null1 -> digestFuture)
                            .whenComplete((_r1, _ex1) -> bufferPool.release(readBuf));

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
        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);

        CompletableFuture<Void> digestFuture = readCompleteFuture.thenCompose(_null -> putParameters.digest.update(readBuf));

        CompletableFuture<Void> writeFuture = readCompleteFuture.thenCompose(_null -> {
            if(readBuf.remaining() == 0) {
                bufferPool.release(readBuf);
                return CompletableFuture.<Void>completedFuture(null);
            }
            return tx.update(readBuf, objectOffset, readBuf.remaining())
                    .thenCompose(_null1 -> digestFuture)
                    .whenComplete((r, ex) -> bufferPool.release(readBuf));
        });

        CompletableFuture<Void> readNextFuture = readCompleteFuture.thenCompose(readLength -> {
            if(readBuf.remaining() < putParameters.objectSize)
                return CompletableFuture.<Void>completedFuture(null);
            return digestFuture.thenCompose(_digestCompletion -> putSequence(putParameters, tx, objectOffset + 1));
        });

        return CompletableFuture.allOf(writeFuture, readNextFuture);
    }

    public <T> CompletableFuture<T> amUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<AmService.AsyncIface>, CompletableFuture<T>> operation) {
        if(authorizer.hasAccess(token, volume))
            return amPool.use(operation);
        else
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
    }

    public <T> CompletableFuture<T> csUseVolume(String volume, AsyncResourcePool.BindWithException<XdiClientConnection<ConfigurationService.AsyncIface>, CompletableFuture<T>> operation) {
        if(authorizer.hasAccess(token, volume))
            return csPool.use(operation);
        else
            return CompletableFutureUtility.exceptionFuture(new SecurityException());
    }

    public CompletableFuture<VolumeDescriptor> statVolume(String domain, String volume) {
        return csUseVolume(volume, cs -> {
            CompletableFuture<VolumeDescriptor> result = new CompletableFuture<>();
            cs.getClient().statVolume(domain, volume, makeThriftCallbacks(result, r -> r.getResult()));
            return result;
        });
    }

    public CompletableFuture<BlobDescriptor> statBlob(String domain, String volume, String blob) {
        return amUseVolume(volume, am -> {
            CompletableFuture<BlobDescriptor> result = new CompletableFuture<>();
            am.getClient().statBlob(domain, volume, blob, makeThriftCallbacks(result, r -> r.getResult()));
            return result;
        });
    }

    public CompletableFuture<ByteBuffer> getBlob(String domain, String volume, String blob, long offset, int length) {
        return amUseVolume(volume, am -> {
            CompletableFuture<ByteBuffer> result = new CompletableFuture<>();
            am.getClient().getBlob(domain, volume, blob, length, new ObjectOffset(offset), makeThriftCallbacks(result, r -> r.getResult()));
            return result;
        });
    }

    public CompletableFuture<Void> updateBlobOnce(String domain, String volume, String blob, int blobMode, ByteBuffer data, int length, long offset, Map<String, String> metadata) {
        return amUseVolume(volume, am -> {
            CompletableFuture<Void> result = new CompletableFuture<>();
            am.getClient().updateBlobOnce(domain, volume, blob, blobMode, data, length, new ObjectOffset(offset), metadata, makeThriftCallbacks(result, r -> null));
            return result;
        });
    }

    public CompletableFuture<TransactionHandle> createTx(String domain, String volume, String blob, int mode) {
        return amUseVolume(volume, am -> {
            CompletableFuture<TransactionHandle> result = new CompletableFuture<>();
            am.getClient().startBlobTx(domain, volume, blob, mode, makeThriftCallbacks(result, r -> new TransactionHandle(domain, volume, blob, r.getResult())));
            return result;
        });
    }

    public class PutResult {
        public byte[] digest;

        public PutResult(byte[] md) {
            digest = md;
        }
    }

    public class TransactionHandle {
        private final String domain;
        private final String volume;
        private final String blob;
        private final TxDescriptor descriptor;

        private TransactionHandle(String domain, String volume, String blob, TxDescriptor descriptor) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            this.descriptor = descriptor;
        }

        public CompletableFuture<Void> commit() {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = new CompletableFuture<>();
                am.getClient().commitBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }

        public CompletableFuture<Void> abort() {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = new CompletableFuture<>();
                am.getClient().abortBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
                return result;
            });

        }

        public CompletableFuture<Void> update(ByteBuffer buffer, long objectOffset, int length) {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = new CompletableFuture<>();
                am.getClient().updateBlob(domain, volume, blob, descriptor, buffer, length, new ObjectOffset(objectOffset), false, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }

        public CompletableFuture<Void> updateMetadata(Map<String, String> meta) {
            return amUseVolume(volume, am -> {
                CompletableFuture<Void> result = new CompletableFuture<Void>();
                am.getClient().updateMetadata(domain, volume, blob, descriptor, meta, makeThriftCallbacks(result, r -> null));
                return result;
            });
        }
    }

    // TODO: factor this stuff into somewhere more accessible
    private interface FunctionWithExceptions<TIn, TOut> {
        public TOut apply(TIn input) throws Exception;
    }

    private <T> CompletableFuture<T> thenComposeExceptionally(CompletableFuture<T> future, Function<Throwable, CompletableFuture<Void>> handler) {
        CompletableFuture<T> result = new CompletableFuture<>();

        future.whenComplete((r, ex) -> {
            if(ex == null)
                result.complete(r);
            else
                handler.apply(ex).thenApply(_null -> result.completeExceptionally(ex));
        });

        return result;
    }

    private static <T,R> AsyncMethodCallback<T> makeThriftCallbacks(CompletableFuture<R> future, FunctionWithExceptions<T, R> extractor) {
        return new AsyncMethodCallback<T>() {
            @Override
            public void onComplete(T result) {
                try {
                    future.complete(extractor.apply(result));
                } catch(Exception ex) {
                    future.completeExceptionally(ex);
                }
            }

            @Override
            public void onError(Exception e) {
                future.completeExceptionally(e);
            }
        };
    }

    private static class PutParameters {
        public final String domain;
        public final String volume;
        public final String blob;
        public final Function<ByteBuffer, CompletableFuture<Void>> reader;
        public final int objectSize;
        public final AsyncMessageDigest digest;
        public final Map<String, String> metadata;

        private PutParameters(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader, int objectSize, AsyncMessageDigest digest) {
            this.domain = domain;
            this.volume = volume;
            this.blob = blob;
            this.reader = reader;
            this.objectSize = objectSize;
            this.digest = digest;
            metadata = new HashMap<>();
        }

        private CompletableFuture<Map<String,String>> getFinalizedMetadata() {
            HashMap<String, String> md = new HashMap<>(metadata);
            md.computeIfAbsent(Xdi.LAST_MODIFIED, lm -> Long.toString(DateTime.now().getMillis()));
            return digest.get()
                    .thenApply(bytes -> {
                        md.put("etag", Hex.encodeHexString(bytes));
                        return md;
                    });
        }
    }

    public static class Factory {
        private final Authorizer authorizer;
        private final AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool;
        private final AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool;
        private final ByteBufferPool bufferPool;

        public Factory(Authorizer authorizer,
                               AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> csPool,
                               AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> amPool,
                               ByteBufferPool bufferPool) {
            this.authorizer = authorizer;
            this.csPool = csPool;
            this.amPool = amPool;
            this.bufferPool = bufferPool;
        }

        public XdiAsync createAuthenticated(AuthenticationToken token) {
            return new XdiAsync(authorizer, csPool, amPool, bufferPool, token);
        }
    }
}


