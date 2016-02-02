package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Optional;

public class Chunker {
    private IoOps io;

    public Chunker(IoOps io) {
        this.io = io;
    }

    public void write(String domain, String volume, String blobName, int objectSize, byte[] bytes, long offset, int length, MetadataMutator mutator) throws IOException {
        int actualLength = Math.min(bytes.length, length);
        if (actualLength == 0) {
            return;
        }

        long startObject = Math.floorDiv(offset, objectSize);
        final int[] startOffset = new int[]{(int) (offset % objectSize)};
        final int[] remaining = new int[]{length};

        for (long i = startObject; remaining[0] > 0; i++) {
            int toBeWritten = Math.min(objectSize - startOffset[0], remaining[0]);
            ObjectOffset objectOffset = new ObjectOffset(i);

            io.readMetadata(domain, volume, blobName).orElse(new FdsMetadata()).lock(m -> {
                mutator.mutate(m.mutableMap());
                io.readCompleteObject(domain, volume, blobName, objectOffset, objectSize).lock(o -> {
                    int limit = Math.max(o.limit(), startOffset[0] + toBeWritten);
                    o.limit(limit);
                    ByteBuffer bb = o.asByteBuffer();
                    bb.position(startOffset[0]);
                    bb.put(bytes, actualLength - remaining[0], toBeWritten);
                    startOffset[0] = 0;
                    remaining[0] -= toBeWritten;
                    io.writeObject(domain, volume, blobName, objectOffset, o.fdsObject(), true);
                    return null;
                });

                io.writeMetadata(domain, volume, blobName, m.fdsMetadata(), true);
                return null;
            });
        }
    }

    public int read(String domain, String volume, String blobName, int objectSize, byte[] destination, long offset, int length) throws IOException {
        final int[] remaining = new int[]{Math.min(destination.length, length)};
        if (remaining[0] == 0) {
            return 0;
        }

        long totalObjects = (long) Math.ceil((double) (length + (offset % objectSize)) / (double) objectSize);
        long startObject = Math.floorDiv(offset, objectSize);
        final int[] startOffset = new int[]{(int) (offset % objectSize)};
        final int[] readSoFar = new int[]{0};
        ByteBuffer output = ByteBuffer.wrap(destination);

        for (long i = 0; i < totalObjects; i++) {
            Optional<FdsMetadata> fdsMetadata = io.readMetadata(domain, volume, blobName);
            if (!fdsMetadata.isPresent()) {
                throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
            }
            ObjectOffset objectOffset = new ObjectOffset(startObject + i);
            fdsMetadata.get().lock(m -> {
                io.readCompleteObject(domain, volume, blobName, objectOffset, objectSize).lock(o -> {
                    int toBeRead = Math.min(o.limit() - startOffset[0], (objectSize - startOffset[0]));
                    toBeRead = Math.min(toBeRead, output.remaining());
                    ByteBuffer buf = o.asByteBuffer();
                    buf.position(startOffset[0]);
                    buf.limit(startOffset[0] + toBeRead);
                    output.put(buf);
                    startOffset[0] = 0;
                    remaining[0] -= toBeRead;
                    readSoFar[0] += toBeRead;
                    return null;
                });
                return null;
            });
        }

        return readSoFar[0];
    }

}
