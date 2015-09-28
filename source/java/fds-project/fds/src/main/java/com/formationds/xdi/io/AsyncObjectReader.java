package com.formationds.xdi.io;

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.util.ByteBufferUtility;
import com.formationds.util.async.AsyncFlow;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import com.formationds.xdi.FdsObjectFrame;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Optional;
import java.util.Queue;
import java.util.concurrent.CompletableFuture;

public class AsyncObjectReader {
    private final Iterator<FdsObjectFrame> frameIterator;
    private AsyncAm am;
    private BlobSpecifier specifier;
    private int readAhead;
    private boolean firstRead;

    private ByteBuffer object0;
    private Queue<CompletableFuture<ByteBuffer>> readFutures;
    private CompletableFuture<BlobDescriptor> metadataFuture;

    public AsyncObjectReader(AsyncAm am, BlobSpecifier specifier, Iterable<FdsObjectFrame> readFrames) {
        this.am = am;
        this.specifier = specifier;
        readAhead = 20;
        frameIterator = readFrames.iterator();
        readFutures = new LinkedList<>();
        firstRead = true;
    }

    public AsyncObjectReader(AsyncAm am, BlobSpecifier specifier, Iterable<FdsObjectFrame> readFrames, BlobWithMetadata object0Read) {
        this(am, specifier, readFrames);
        firstRead = false;
        metadataFuture = CompletableFuture.completedFuture(object0Read.getBlobDescriptor());
        object0 = object0Read.getBytes();
    }

    public Optional<CompletableFuture<ByteBuffer>> getNextObject() {
        synchronized (readFutures) {
            fillReadQueue();
            if(readFutures.isEmpty())
                return Optional.empty();
            return Optional.of(readFutures.remove());
        }
    }

    public CompletableFuture<Void> readToStream(OutputStream stream) {
        return AsyncFlow.loop(loopHandle -> {
            Optional<CompletableFuture<ByteBuffer>> nextObject = getNextObject();
            if(!nextObject.isPresent())
                return loopHandle.complete(null);

            return nextObject.get().thenComposeAsync(buf -> {
                try {
                    ByteBufferUtility.writeFromByteBuffer(buf, stream);
                    return CompletableFuture.completedFuture(null);
                } catch (IOException e) {
                    return CompletableFutureUtility.exceptionFuture(e);
                }
            });
        });
    }

    public CompletableFuture<BlobDescriptor> getDescriptor() {
        synchronized (readFutures) {
            fillReadQueue();
            return metadataFuture;
        }

    }

    private void readCompletion() {
        synchronized (readFutures) {
            fillReadQueue();
        }
    }

    private void fillReadQueue() {
        while(readFutures.size() < readAhead && frameIterator.hasNext()) {
            FdsObjectFrame frame = frameIterator.next();

            CompletableFuture<ByteBuffer> objectRead;
            if(firstRead) {
                CompletableFuture<BlobWithMetadata> blobWithMeta = am.getBlobWithMeta(specifier.getDomainName(), specifier.getVolumeName(), specifier.getBlobName(), frame.internalLength + frame.internalOffset, new ObjectOffset(frame.objectOffset));
                objectRead = blobWithMeta.thenApply(BlobWithMetadata::getBytes);
                metadataFuture = blobWithMeta.thenApply(BlobWithMetadata::getBlobDescriptor);
            } else if(frame.objectOffset == 0L && object0 != null) {
                objectRead = CompletableFuture.completedFuture(object0.duplicate());
            } else {
                objectRead = am.getBlob(specifier.getDomainName(), specifier.getVolumeName(), specifier.getBlobName(), frame.internalLength + frame.internalOffset, new ObjectOffset(frame.objectOffset));
            }
            objectRead = objectRead.thenApply(buf -> {
                buf.limit(buf.position() + frame.internalOffset + frame.internalLength);
                buf.position(buf.position() + frame.internalOffset);
                return buf;
            });

            readFutures.add(objectRead);
            objectRead.whenComplete((c, x) -> readCompletion());
            object0 = null;
        }
    }
}
