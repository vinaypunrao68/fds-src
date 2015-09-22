package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;

public class Chunker {
    private Io io;

    public Chunker(Io io) {
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
            io.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(i), (ov) -> {
                mutator.mutate(ov.getMetadata());
                ByteBuffer buf = ov.getBuf().slice();
                buf.position(startOffset[0]);
                buf.put(bytes, actualLength - remaining[0], toBeWritten);
                buf.position(0);
                startOffset[0] = 0;
                remaining[0] -= toBeWritten;
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
            io.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(startObject + i), oov -> {
                if (!oov.isPresent()) {
                    throw new FileNotFoundException();
                }
                ByteBuffer buf = oov.get().getBuf();
                int toBeRead = Math.min(buf.remaining() - startOffset[0], (objectSize - startOffset[0]));
                toBeRead = Math.min(toBeRead, output.remaining());
                buf.position(buf.position() + startOffset[0]);
                buf.limit(buf.position() + toBeRead);
                output.put(buf);
                startOffset[0] = 0;
                remaining[0] -= toBeRead;
                readSoFar[0] += toBeRead;
                return null;
            });
        }

        return readSoFar[0];
    }

}
