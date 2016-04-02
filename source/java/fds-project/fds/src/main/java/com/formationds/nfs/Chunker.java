package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.Retry;
import com.formationds.xdi.FdsObjectFrame;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joda.time.Duration;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class Chunker {
    private static final Logger LOG = LogManager.getLogger(Chunker.class);
    private IoOps io;
    private final StripedLock metaLock, objectLock;

    public Chunker(IoOps io) {
        this.io = io;
        metaLock = new StripedLock();
        objectLock = new StripedLock();
    }

    public <T> T write(String domain, String volume, String blobName, int maxObjectSize, byte[] bytes, long offset, int length, MetadataMutator<T> mutator) throws IOException {
        int actualLength = Math.min(bytes.length, length);
        if (actualLength == 0) {
            Map<String, String> metadata = io.readMetadata(domain, volume, blobName).orElse(new HashMap<>());
            return mutator.mutate(metadata);
        }

        long startObject = Math.floorDiv(offset, maxObjectSize);
        final int[] startOffset = new int[]{(int) (offset % maxObjectSize)};
        final int[] remaining = new int[]{length};

        for (long i = startObject; remaining[0] > 0; i++) {
            int toBeWritten = Math.min(maxObjectSize - startOffset[0], remaining[0]);
            ObjectOffset objectOffset = new ObjectOffset(i);
            ObjectKey key = new ObjectKey(domain, volume, blobName, objectOffset);
            objectLock.lock(key, () -> {
                FdsObject fdsObject;

                if (toBeWritten == maxObjectSize) {
                    fdsObject = new FdsObject(ByteBuffer.allocate(maxObjectSize), maxObjectSize);
                } else {
                    fdsObject = io.readCompleteObject(domain, volume, blobName, objectOffset, maxObjectSize);
                }
                int limit = Math.max(fdsObject.limit(), startOffset[0] + toBeWritten);
                fdsObject.limit(limit);
                ByteBuffer bb = fdsObject.asByteBuffer();
                bb.position(startOffset[0]);
                bb.put(bytes, actualLength - remaining[0], toBeWritten);
                startOffset[0] = 0;
                remaining[0] -= toBeWritten;
                io.writeObject(domain, volume, blobName, objectOffset, fdsObject);
                return null;
            });
        }

        return metaLock.lock(new MetaKey(domain, volume, blobName), () -> {
            Map<String, String> metadata = io.readMetadata(domain, volume, blobName).orElse(new HashMap<>());
            T result = mutator.mutate(metadata);
            io.writeMetadata(domain, volume, blobName, metadata);
            return result;
        });
    }

    public int read(String domain, String volume, String blobName, long blobLength, int maxObjectSize, byte[] destination, long offset, int length) throws IOException {
        if (offset > blobLength) {
            return 0;
        }

        long readLength = Math.min(length, destination.length);
        readLength = Math.min(readLength, blobLength - offset);

        ByteBuffer output = ByteBuffer.wrap(destination);
        Optional<Map<String, String>> opt = io.readMetadata(domain, volume, blobName);

        if (!opt.isPresent()) {
            throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
        }

        FdsObjectFrame.FrameIterator iterator = new FdsObjectFrame.FrameIterator(offset, readLength, maxObjectSize);

        while (iterator.hasNext()) {
            FdsObjectFrame frame = iterator.next();
            ObjectKey objectKey = new ObjectKey(domain, volume, blobName, new ObjectOffset(frame.objectOffset));
            objectLock.lock(objectKey, () -> {
                // Work around a probable and obscure AM caching issue
                Retry retry = new Retry((x, y) -> {
                    FdsObject fdsObject = io.readCompleteObject(domain, volume, blobName, new ObjectOffset(frame.objectOffset), maxObjectSize);
                    output.put(fdsObject.bytes(), frame.internalOffset, frame.internalLength);
                    return null;
                }, 5, Duration.millis(10), "object read");

                try {
                    return retry.apply(null);
                } catch (Exception e) {
                    LOG.error("Error reading object", e);
                    throw new IOException(e);
                }
            });
        }

        return output.limit();
    }

}
