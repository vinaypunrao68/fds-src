package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.blob.Mode;
import com.formationds.web.toolkit.ServletOutputWrapper;
import org.apache.thrift.TException;
import org.apache.thrift.async.AsyncMethodCallback;
import org.eclipse.jetty.io.ByteBufferPool;

import javax.servlet.ServletOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class XdiAsync {
    private final ConfigurationService.AsyncIface cs;
    private ByteBufferPool pool;
    private AmService.AsyncIface am;

    public XdiAsync(AmService.AsyncIface am, ConfigurationService.AsyncIface cs, ByteBufferPool pool) {
        this.am = am;
        this.cs = cs;
        this.pool = pool;
    }

    public CompletableFuture<Void> getBlobToServlet(String domain, String volume, String blob, ServletOutputStream stream) {
        ServletOutputWrapper sow = new ServletOutputWrapper(stream);
        return getBlobToConsumer(domain, volume, blob, bytes -> {
                    try {
                        return sow.outAsync(str -> str.write(bytes.array()));
                    } catch(IOException ex) {
                        return makeExceptionFuture(ex);
                    }
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


    public CompletableFuture<Void> putBlobFromSupplier(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader) {
        return statVolume(domain, volume).thenCompose(volumeDescriptor -> firstPut(domain, volume, blob, reader, volumeDescriptor.getPolicy().getMaxObjectSizeInBytes(), 0));
    }

    private CompletableFuture<Void> firstPut(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader, int objectSize, long objectOffset) {
        ByteBuffer readBuf = pool.acquire(objectSize, false);
        CompletableFuture<Integer> readCompleteFuture = reader.apply(readBuf).thenApply(_null -> rewindAndGetPosition(readBuf));

        return readCompleteFuture.thenCompose(length -> {
            if(length < objectSize) {
                return updateBlobOnce(domain, volume, blob, readBuf, Mode.TRUNCATE.getValue(), length, objectOffset, null).whenComplete((_null, ex) -> pool.release(readBuf));
            } else {
                return secondPut(domain, volume, blob, reader, objectSize, objectOffset + 1, readBuf);
            }
        });
    }

    private CompletableFuture<Void> secondPut(String domain, String volume, String blob, Function<ByteBuffer, CompletableFuture<Void>> reader, int objectSize, long objectOffset, ByteBuffer first) {
        ByteBuffer readBuf = pool.acquire(objectSize, false);
        CompletableFuture<Integer> readCompleteFuture = reader.apply(readBuf).thenApply(_null -> rewindAndGetPosition(readBuf));
        return readCompleteFuture.thenCompose(length -> {
            if(length == 0) {
                return updateBlobOnce(domain, volume, blob, first, Mode.TRUNCATE.getValue(), objectSize, objectOffset, null).whenComplete((_null, ex) -> pool.release(readBuf));
            } else {
                return createTx(domain, volume, blob, Mode.TRUNCATE.getValue()).thenCompose(tx -> {
                    CompletableFuture<Void> firstResult = tx.update(first, objectOffset - 1, objectSize).whenComplete((_r1, _ex1) -> pool.release(first));
                    CompletableFuture<Void> secondResult = tx.update(readBuf, objectOffset, length).whenComplete((_r1, _ex1) -> pool.release(readBuf));
                    CompletableFuture<Void> rest = putSequence(tx, reader, objectSize, objectOffset + 1);

                    return completeTransaction(tx, CompletableFuture.allOf(firstResult, secondResult, rest));
                });
            }
        });
    }

    private CompletableFuture<Void> completeTransaction(TransactionHandle tx, CompletableFuture<Void> priorFuture) {
        return thenComposeExceptionally(priorFuture.thenCompose(_null -> tx.commit()), _null -> tx.abort());
    }

    private CompletableFuture<Void> putSequence(TransactionHandle tx, Function<ByteBuffer, CompletableFuture<Void>> reader, int objectSize, long objectOffset) {
        ByteBuffer readBuf = pool.acquire(objectSize, false);
        CompletableFuture<Integer> readCompleteFuture = reader.apply(readBuf).thenApply(_null -> rewindAndGetPosition(readBuf));

        CompletableFuture<Void> writeFuture = readCompleteFuture.thenCompose(readLength -> {
            if(readLength == 0) {
                pool.release(readBuf);
                return CompletableFuture.<Void>completedFuture(null);
            }
            return tx.update(readBuf, objectOffset, readLength).whenComplete((r, ex) -> pool.release(readBuf));
        });

        CompletableFuture<Void> readNextFuture = readCompleteFuture.thenCompose(readLength -> {
            if(readLength < objectSize)
                return CompletableFuture.<Void>completedFuture(null);
            return putSequence(tx, reader, objectSize, objectOffset + 1);
        });

        return CompletableFuture.allOf(writeFuture, readNextFuture);
    }

    private int rewindAndGetPosition(ByteBuffer buffer) {
        int position = buffer.position();
        buffer.rewind();
        return position;
    }

    public CompletableFuture<VolumeDescriptor> statVolume(String domain, String volume) {
        CompletableFuture<VolumeDescriptor> result = new CompletableFuture<>();
        try {
            cs.statVolume(domain, volume, makeThriftCallbacks(result, r -> r.getResult()));
        } catch(TException ex) {
            result.completeExceptionally(ex);
        }
        return result;
    }

    public CompletableFuture<BlobDescriptor> statBlob(String domain, String volume, String blob) {
        CompletableFuture<BlobDescriptor> result = new CompletableFuture<>();
        try {
            am.statBlob(domain, volume, blob, makeThriftCallbacks(result, r -> r.getResult()));
        } catch (TException ex) {
            result.completeExceptionally(ex);
        }
        return result;
    }

    public CompletableFuture<ByteBuffer> getBlob(String domain, String volume, String blob, long offset, int length) {
        CompletableFuture<ByteBuffer> result = new CompletableFuture<>();
        try {
            am.getBlob(domain, volume, blob, length, new ObjectOffset(offset), makeThriftCallbacks(result, r -> r.getResult()));
        } catch(TException ex) {
            result.completeExceptionally(ex);
        }
        return result;
    }

    public CompletableFuture<Void> updateBlobOnce(String domain, String volume, String blob, ByteBuffer data, int blobMode, int length, long offset, Map<String, String> metadata) {
        CompletableFuture<Void> result = new CompletableFuture<>();
        try {
            am.updateBlobOnce(domain, volume, blob, blobMode, data, length, new ObjectOffset(offset), metadata, makeThriftCallbacks(result, r -> null));
        } catch(TException ex) {
            result.completeExceptionally(ex);
        }
        return result;
    }

    public CompletableFuture<TransactionHandle> createTx(String domain, String volume, String blob, int mode) {
        CompletableFuture<TransactionHandle> result = new CompletableFuture<>();
        try {
            am.startBlobTx(domain, volume, blob, mode, makeThriftCallbacks(result, r -> new TransactionHandle(domain, volume, blob, r.getResult())));
        } catch(TException ex) {
            result.completeExceptionally(ex);
        }
        return result;
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
            CompletableFuture<Void> result = new CompletableFuture<>();
            try {
                am.commitBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
            } catch(TException ex) {
                result.completeExceptionally(ex);
            }
            return result;
        }

        public CompletableFuture<Void> abort() {
            CompletableFuture<Void> result = new CompletableFuture<>();
            try {
                am.abortBlobTx(domain, volume, blob, descriptor, makeThriftCallbacks(result, r -> null));
            } catch(TException ex) {
                result.completeExceptionally(ex);
            }
            return result;
        }

        public CompletableFuture<Void> update(ByteBuffer buffer, long objectOffset, int length) {
            CompletableFuture<Void> result = new CompletableFuture<>();
            try {
                am.updateBlob(domain, volume, blob, descriptor, buffer, length, new ObjectOffset(objectOffset), false, makeThriftCallbacks(result, r -> null));
            } catch(TException ex) {
                result.completeExceptionally(ex);
            }
            return result;
        }

        public CompletableFuture<Void> updateMetadata(Map<String, String> meta) {
            CompletableFuture<Void> result = new CompletableFuture<>();
            try {
                am.updateMetadata(domain, volume, blob, descriptor, meta, makeThriftCallbacks(result, r -> null));
            } catch(TException ex) {
                result.completeExceptionally(ex);
            }
            return result;
        }
    }


    // TODO: factor this stuff into somewhere more accessible
    private interface FunctionWithExceptions<TIn, TOut> {
        public TOut apply(TIn input) throws Exception;
    }

    private <T> CompletableFuture<T> makeExceptionFuture(Throwable ex) {
        CompletableFuture<T> cf = new CompletableFuture<>();
        cf.completeExceptionally(ex);
        return cf;
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
}


