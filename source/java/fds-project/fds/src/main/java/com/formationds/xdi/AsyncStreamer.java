package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.DigestUtil;
import com.formationds.util.async.AsyncMessageDigest;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.blob.Mode;
import org.apache.commons.codec.binary.Hex;
import org.apache.thrift.TException;
import org.eclipse.jetty.io.ByteBufferPool;
import org.joda.time.DateTime;

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

public class AsyncStreamer {
    private AsyncAm asyncAm;
    private XdiConfigurationApi configurationApi;
    private AsyncRequestStatistics statistics;

    public AsyncStreamer(AsyncAm asyncAm,
                         XdiConfigurationApi configurationApi) {
        this.asyncAm = asyncAm;
        this.configurationApi = configurationApi;
        statistics = new AsyncRequestStatistics();
    }

    public CompletableFuture<BlobInfo> getBlobInfo(String domain, String volume, String blob) {
        CompletableFuture<VolumeDescriptor> volumeDescriptorFuture = statVolume(domain, volume);
        CompletableFuture<BlobWithMetadata> getObject0Future = volumeDescriptorFuture
                .thenCompose(vd -> asyncAm.getBlobWithMeta(domain, volume, blob, vd.getPolicy().getMaxObjectSizeInBytes(), new ObjectOffset((long) 0)));
        return getObject0Future.thenCompose(bwm ->
                volumeDescriptorFuture
                        .thenApply(vd -> new BlobInfo(domain, volume, blob,
                                bwm.getBlobDescriptor(), vd,
                                bwm.getBytes())));
    }

    public CompletableFuture<PutResult> putBlobFromStream(String domain, String volume, String blob, Map<String, String> metadata, InputStream stream) {
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

    public CompletableFuture<Void> readToOutputStream(BlobInfo blob, OutputStream str, long offset, long byteCount) {
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
        }, offset, byteCount);
    }

    private CompletableFuture<Void> getBlobToConsumer(BlobInfo blobInfo, Function<ByteBuffer, CompletableFuture<Void>> processor, long offset, long length) {
        int objectSize = blobInfo.getVolumeDescriptor().getPolicy().getMaxObjectSizeInBytes();

        // TODO: do we need to worry about limiting reads?
        ArrayList<CompletableFuture<ByteBuffer>> readFutures = new ArrayList<>();
        for (FdsObjectFrame frame : FdsObjectFrame.frames(offset, length, objectSize)) {
            if (frame.objectOffset == 0)
                readFutures.add(CompletableFuture.completedFuture(blobInfo.getObject0()));
            else
                readFutures.add(asyncAm.getBlob(blobInfo.getDomain(), blobInfo.getVolume(), blobInfo.getBlob(), frame.internalLength, new ObjectOffset(frame.objectOffset)));
        }

        CompletableFuture<Void> result = CompletableFuture.completedFuture(null);
        for (CompletableFuture<ByteBuffer> readFuture : readFutures) {
            result = result.thenCompose(r -> readFuture.thenCompose(buf -> processor.apply(buf)));
        }
        return result;
    }

    public CompletableFuture<PutResult> putBlobFromSupplier(String domain, String volume, String blob, Map<String, String> metadata, Function<ByteBuffer, CompletableFuture<Void>> reader) {
        AsyncMessageDigest asmd = new AsyncMessageDigest(DigestUtil.newMd5());
        return statVolume(domain, volume)
                .thenCompose(volumeDescriptor -> firstPut(new PutParameters(domain, volume, blob, reader, volumeDescriptor.getPolicy().getMaxObjectSizeInBytes(), asmd, metadata), 0))
                .thenCompose(_completion -> asmd.get())
                .thenApply(digestBytes -> new PutResult(digestBytes));
    }

    private ByteBuffer condenseBuffer(ByteBuffer buffer) {
        if (buffer.remaining() < buffer.capacity() / 2) {
            ByteBuffer targetBuffer = ByteBuffer.allocate(buffer.remaining());
            targetBuffer.limit(targetBuffer.capacity());
            targetBuffer.put(buffer);
            targetBuffer.flip();
            return targetBuffer;
        }
        return buffer;
    }

    private CompletableFuture<Void> firstPut(PutParameters putParameters, long objectOffset) {
        ByteBuffer readBuf = ByteBuffer.allocate(putParameters.objectSize);
        readBuf.limit(putParameters.objectSize);
        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);

        return readCompleteFuture.thenCompose(_null -> {
            final ByteBuffer condensedBuffer = condenseBuffer(readBuf);
            putParameters.digest.update(condensedBuffer);
            if (condensedBuffer.remaining() < putParameters.objectSize) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                        updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), condensedBuffer, condensedBuffer.remaining(), objectOffset, md));
            } else {
                // secondPut is called for the put requests equal to or larger than the objectSize.
                return secondPut(putParameters, objectOffset + 1, condensedBuffer);
            }
        });
    }

    private CompletableFuture<Void> secondPut(PutParameters putParameters, long objectOffset, ByteBuffer first) {
        ByteBuffer readBuf = ByteBuffer.allocate(putParameters.objectSize);
        readBuf.limit(putParameters.objectSize);

        CompletableFuture<Void> readCompleteFuture = putParameters.reader.apply(readBuf);
        return readCompleteFuture.thenCompose(_null -> {
            final ByteBuffer condensedBuffer = condenseBuffer(readBuf);
            if (condensedBuffer.remaining() == 0) {
                return putParameters.getFinalizedMetadata().thenCompose(md ->
                        updateBlobOnce(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue(), first, putParameters.objectSize, objectOffset - 1, md));
            } else {
                CompletableFuture<Void> digestFuture = putParameters.digest.update(condensedBuffer);
                return createTx(putParameters.domain, putParameters.volume, putParameters.blob, Mode.TRUNCATE.getValue()).thenComposeAsync(tx -> {
                    CompletableFuture<Void> firstResult = tx.update(first, objectOffset - 1, putParameters.objectSize)
                            .thenCompose(_null1 -> digestFuture);
                    CompletableFuture<Void> secondResult = tx.update(condensedBuffer, objectOffset, condensedBuffer.remaining())
                            .thenCompose(_null1 -> digestFuture);

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
        ByteBuffer readBuf = ByteBuffer.allocate(putParameters.objectSize);
        readBuf.limit(putParameters.objectSize);
        CompletableFuture<ByteBuffer> readCompleteFuture = putParameters.reader.apply(readBuf).thenApply(_null -> condenseBuffer(readBuf));

        return readCompleteFuture.thenCompose(condensedBuffer -> {
            CompletableFuture<Void> digestFuture = putParameters.digest.update(condensedBuffer);
            CompletableFuture<Void> writeFuture = null;
            CompletableFuture<Void> readNextFuture = null;
            if (condensedBuffer.remaining() == 0) {
                writeFuture = CompletableFuture.<Void>completedFuture(null);
            } else {
                writeFuture = tx.update(condensedBuffer, objectOffset, condensedBuffer.remaining())
                        .thenCompose(_null1 -> digestFuture);
            }

            if (condensedBuffer.remaining() < putParameters.objectSize)
                readNextFuture = CompletableFuture.<Void>completedFuture(null);
            else
                readNextFuture = digestFuture.thenCompose(_digestCompletion -> putSequence(putParameters, tx, objectOffset + 1));

            return CompletableFuture.allOf(readNextFuture, writeFuture);
        });
    }


    public CompletableFuture<VolumeDescriptor> statVolume(String domain, String volume) {
        CompletableFuture<VolumeDescriptor> cf = new CompletableFuture<>();
        try {
            VolumeDescriptor volumeDescriptor = configurationApi.statVolume(domain, volume);
            cf.complete(volumeDescriptor);
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }

        return cf;
    }

    public CompletableFuture<BlobDescriptor> statBlob(String domain, String volume, String blob) {
        return asyncAm.statBlob(domain, volume, blob);
    }

    public CompletableFuture<BlobDescriptor> renameBlob(String domain, String volume, String sourceBlobName, String destinationBlobName) {
        return asyncAm.renameBlob(domain, volume, sourceBlobName, destinationBlobName);
    }

    private CompletableFuture<Void> updateBlobOnce(String domain, String volume, String blob, int blobMode, ByteBuffer data, int length, long offset, Map<String, String> metadata) {
        return asyncAm.updateBlobOnce(domain, volume, blob, blobMode, data, length, new ObjectOffset(offset), metadata);
    }

    private CompletableFuture<TransactionHandle> createTx(String domain, String volume, String blob, int mode) {
        return asyncAm.startBlobTx(domain, volume, blob, mode)
                .thenApply(tx -> new TransactionHandle(domain, volume, blob, tx));
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

    public OutputStream openForWriting(String domainName, String volumeName, String blobName, Map<String, String> metadata) throws Exception {
        VolumeDescriptor volumeDescriptor = statVolume(domainName, volumeName).get();
        int chunkSize = volumeDescriptor.getPolicy().getMaxObjectSizeInBytes();
        TxDescriptor tx = asyncAm.startBlobTx(domainName, volumeName, blobName, 1).get();
        MessageDigest messageDigest = MessageDigest.getInstance("MD5");
        return new OutputStream() {
            ByteBuffer buf = ByteBuffer.allocate(chunkSize);
            long chunksWrittenSoFar = 0;

            @Override
            public void write(int b) throws IOException {
                if (buf.remaining() == 0) {
                    buf.flip();
                    try {
                        asyncAm.updateBlob(domainName, volumeName, blobName, tx, buf, chunkSize, new ObjectOffset(chunksWrittenSoFar), false).get();
                        buf.position(0);
                        buf.limit(chunkSize);
                        chunksWrittenSoFar++;
                    } catch (Exception e) {
                        throw new IOException(e);
                    }
                }
                buf.put((byte) b);
                messageDigest.update((byte) b);
            }

            @Override
            public void close() throws IOException {
                try {
                    if (buf.position() != 0) {
                        int length = buf.position();
                        buf.flip();
                        asyncAm.updateBlob(domainName, volumeName, blobName, tx, buf, length, new ObjectOffset(chunksWrittenSoFar), true).get();
                        metadata.put(Xdi.LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
                        metadata.put("etag", Hex.encodeHexString(messageDigest.digest()));
                        asyncAm.updateMetadata(domainName, volumeName, blobName, tx, metadata).get();
                    }
                    asyncAm.commitBlobTx(domainName, volumeName, blobName, tx).get();
                } catch (Exception e) {
                    throw new IOException(e);
                }
                super.close();
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
            return asyncAm.commitBlobTx(domain, volume, blob, descriptor);
        }

        public CompletableFuture<Void> abort() {
            return asyncAm.abortBlobTx(domain, volume, blob, descriptor);
        }

        public CompletableFuture<Void> update(ByteBuffer buffer, long objectOffset, int length) {
            return asyncAm.updateBlob(domain, volume, blob, descriptor, buffer, length, new ObjectOffset(objectOffset), false);
        }

        public CompletableFuture<Void> updateMetadata(Map<String, String> meta) {
            return asyncAm.updateMetadata(domain, volume, blob, descriptor, meta);
        }
    }

}
