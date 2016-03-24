package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.Retry;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
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

    public int read(String domain, String volume, String blobName, int maxObjectSize, byte[] destination, long offset, int length) throws IOException {
        final int[] remaining = new int[]{Math.min(destination.length, length)};
        if (remaining[0] == 0) {
            return 0;
        }

        long totalObjects = (long) Math.ceil((double) (length + (offset % maxObjectSize)) / (double) maxObjectSize);
        long startObject = Math.floorDiv(offset, maxObjectSize);
        final int[] startOffset = new int[]{(int) (offset % maxObjectSize)};
        final int[] readSoFar = new int[]{0};
        ByteBuffer output = ByteBuffer.wrap(destination);

        for (long i = 0; i < totalObjects; i++) {
            Optional<Map<String, String>> opt = io.readMetadata(domain, volume, blobName);
            if (!opt.isPresent()) {
                throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
            }
            ObjectOffset objectOffset = new ObjectOffset(startObject + i);
            ObjectKey key = new ObjectKey(domain, volume, blobName, objectOffset);
            objectLock.lock(key, () -> {
                FdsObject fdsObject = io.readCompleteObject(domain, volume, blobName, objectOffset, maxObjectSize);
                int maxAvailable = Math.min(fdsObject.limit() - startOffset[0], (maxObjectSize - startOffset[0]));
                int toBeRead = Math.min(maxAvailable, output.remaining());
                try {
                    // Yep, ugly. Workaround for what looks like an AM caching issue
                    new Retry((x, y) -> {
                        ByteBuffer buf = fdsObject.asByteBuffer();
                        buf.limit(startOffset[0] + toBeRead);
                        buf.position(startOffset[0]);
                        output.put(buf);
                        startOffset[0] = 0;
                        remaining[0] -= toBeRead;
                        readSoFar[0] += toBeRead;
                        return null;
                    }, 5, Duration.millis(10), "Chunker read")
                            .apply(null);
                } catch (Exception e) {
                    LOG.error("BEGIN -------------------");
                    LOG.error("Read error, volume=" + volume + ", blobName=" + blobName);
                    LOG.error("maxObjectSize=" + maxObjectSize + ", destination=" + destination.length + "bytes, offset=" + offset + ", length=" + length);
                    LOG.error("Object capacity=" + fdsObject.capacity() + ", object limit=" + fdsObject.limit() + ", startOffset=" + startOffset[0]);
                    LOG.error("To be read: " + toBeRead);
                    LOG.error("Stack trace: ", e);
                    LOG.error("END   -------------------");
                    throw new IOException(e);
                }
                return null;
            });
        }

        return readSoFar[0];
    }

}
