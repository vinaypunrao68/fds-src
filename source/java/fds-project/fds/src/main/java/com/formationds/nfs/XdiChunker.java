package com.formationds.nfs;

import com.formationds.xdi.FdsObjectFrame;
import com.formationds.xdi.contracts.api.Xdi;
import com.formationds.xdi.contracts.api.obj.*;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

public class XdiChunker {
    private Xdi xdi;
    private final StripedLock blobLock;

    public XdiChunker(Xdi xdi) {
        this.xdi = xdi;
        blobLock = new StripedLock();
    }

    public <T> T write(BlobPath blobPath, int maxObjectSize, byte[] bytes, long offset, int length, MetadataMutator<T> mutator) throws Exception {
        ObjectRange objectRange = FdsObjectFrame.objectRange(offset, length, maxObjectSize);
        Blob blob = xdi.readBlob(new ReadBlobRequest(blobPath, new CachePolicy(true, false), objectRange)).get();
        FdsObjectFrame.FrameIterator iterator = new FdsObjectFrame.FrameIterator(offset, length, maxObjectSize);
        AtomicLong writtenSoFar = new AtomicLong(0);

        while (iterator.hasNext()) {
            FdsObjectFrame next = iterator.next();

            blobLock.lockWithAnyException(blobPath, () -> {
                FdsObject fdsObject;

                if (next.internalLength == maxObjectSize) {
                    fdsObject = new FdsObject(ByteBuffer.allocate(maxObjectSize), maxObjectSize);
                } else {
                    ObjectDescriptor objectDescriptor = blob.getObjects().get(next.objectOffset);
                    if (objectDescriptor == null) {
                        fdsObject = new FdsObject(ByteBuffer.allocate(maxObjectSize), maxObjectSize);
                        fdsObject.limit(0);
                    } else {
                        String objectId = objectDescriptor.getObjectId();
                        ByteBuffer byteBuffer = xdi.readObject(objectId).get();
                        fdsObject = new FdsObject(byteBuffer, maxObjectSize);
                    }
                }

                int limit = Math.max(fdsObject.limit(), next.internalOffset + next.internalLength);
                fdsObject.limit(limit);
                ByteBuffer bb = fdsObject.asByteBuffer();
                bb.position(next.internalOffset);
                bb.put(bytes, writtenSoFar.intValue(), next.internalLength);
                writtenSoFar.addAndGet(next.internalLength);
                bb.position(0);
                bb.limit(next.internalOffset + next.internalLength);
                String objectId = xdi.writeObject(new WriteObjectRequest(bb)).get();
                blob.getObjects().put(next.objectOffset, new ObjectDescriptor(objectId, next.internalOffset + next.internalLength));
                return null;
            });
        }

        return blobLock.lockWithAnyException(blobPath, () -> {
            T result = mutator.mutate(blob.getMetadata());
            xdi.writeBlob(new WriteBlobRequest(blob, new CachePolicy(true, true), objectRange)).get();
            return result;
        });
    }

    public int read(BlobPath blobPath, long blobLength, int maxObjectSize, byte[] destination, long offset, int length) throws Exception {
        long readLength = Math.min(length, destination.length);
        readLength = Math.min(readLength, blobLength - offset);
        ObjectRange objectRange = FdsObjectFrame.objectRange(offset, readLength, maxObjectSize);
        Blob blob = xdi.readBlob(new ReadBlobRequest(blobPath, new CachePolicy(true, false), objectRange)).get();
        FdsObjectFrame.FrameIterator iterator = new FdsObjectFrame.FrameIterator(offset, readLength, maxObjectSize);
        AtomicInteger readSoFar = new AtomicInteger(0);

        while (iterator.hasNext()) {
            FdsObjectFrame frame = iterator.next();
            boolean endOfFile = blobLock.lockWithAnyException(blobPath, () -> {
                ObjectDescriptor objectDescriptor = blob.getObjects().get(frame.objectOffset);
                if (objectDescriptor == null) {
                    return true;
                }
                String objectId = objectDescriptor.getObjectId();
                ByteBuffer byteBuffer = xdi.readObject(objectId).get();
                byteBuffer.position(byteBuffer.position() + frame.internalOffset);
                byteBuffer.get(destination, readSoFar.get(), frame.internalLength);
                readSoFar.addAndGet(frame.internalLength);
                return false;
            });

            if (endOfFile) {
                break;
            }
        }

        return readSoFar.get();
    }

}
