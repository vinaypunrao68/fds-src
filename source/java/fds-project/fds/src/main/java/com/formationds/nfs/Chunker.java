package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class Chunker {
    private static final Logger LOG = Logger.getLogger(Chunker.class);
    private IoOps io;
    private final StripedLock lock;

    public Chunker(IoOps io) {
        this.io = io;
        lock = new StripedLock();
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

            FdsObject fdsObject = io.readCompleteObject(domain, volume, blobName, objectOffset, maxObjectSize).lock(o -> {
                int limit = Math.max(o.limit(), startOffset[0] + toBeWritten);
                o.limit(limit);
                ByteBuffer bb = o.asByteBuffer();
                bb.position(startOffset[0]);
                bb.put(bytes, actualLength - remaining[0], toBeWritten);
                startOffset[0] = 0;
                remaining[0] -= toBeWritten;
                return o.fdsObject();
            });

            io.writeObject(domain, volume, blobName, objectOffset, fdsObject);
        }

        return lock.lock(new MetaKey(domain, volume, blobName), () -> {
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
            io.readCompleteObject(domain, volume, blobName, objectOffset, maxObjectSize).lock(o -> {
                try {
                    int toBeRead = Math.min(o.limit() - startOffset[0], (maxObjectSize - startOffset[0]));
                    toBeRead = Math.min(toBeRead, output.remaining());
                    ByteBuffer buf = o.asByteBuffer();
                    buf.limit(startOffset[0] + toBeRead);
                    buf.position(startOffset[0]);
                    output.put(buf);
                    startOffset[0] = 0;
                    remaining[0] -= toBeRead;
                    readSoFar[0] += toBeRead;
                } catch (Exception e) {
                    LOG.error("Read error, volume=" + volume + ", blobName=" + blobName);
                    LOG.error("maxObjectSize=" + maxObjectSize + ", destination=" + destination + "bytes, offset=" + offset + ", length=" + length);
                    LOG.error("Object capacity=" + o.capacity() + ", object limit=" + o.limit());
                }
                return null;
            });
        }

        return readSoFar[0];
    }

}
