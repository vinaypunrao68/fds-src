package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;

public class Chunker {
    private ChunkIo io;

    public Chunker(ChunkIo io) {
        this.io = io;
    }

    public void write(NfsPath nfsPath, int objectSize, byte[] bytes, long offset, int length) throws Exception {
        if (length == 0) {
            return;
        }

        int totalObjects = Math.floorDiv(length, objectSize) + 1;
        long startObject = Math.floorDiv(offset, objectSize);
        int startOffset = (int) (offset % objectSize);
        int writtenSoFar = 0;

        for (long i = 0; i < totalObjects; i++) {
            int toBeWritten = Math.min(length, (objectSize - startOffset));
            ByteBuffer newChunk = null;
            try {
                ByteBuffer existing = io.read(nfsPath, objectSize, new ObjectOffset(startObject + i));
                newChunk = ByteBuffer.allocate(Math.max(existing.remaining(), startOffset + toBeWritten));
                newChunk.put(existing);
                newChunk.position(0);

            } catch (FileNotFoundException e) {
                newChunk = ByteBuffer.allocate(startOffset + toBeWritten);
            }

            newChunk.position(startOffset);
            newChunk.put(bytes, writtenSoFar, toBeWritten);
            newChunk.position(0);
            newChunk.limit(startOffset + toBeWritten);
            io.write(nfsPath, objectSize, new ObjectOffset(startObject + i), newChunk);
            startOffset = 0;
            length -= toBeWritten;
            writtenSoFar += toBeWritten;
        }
    }

    public int read(NfsPath nfsPath, int objectSize, byte[] destination, long offset, int length) throws Exception {
        if (length == 0) {
            return 0;
        }

        int totalObjects = Math.floorDiv(length, objectSize) + 1;
        long startObject = Math.floorDiv(offset, objectSize);
        int startOffset = (int) (offset % objectSize);
        int readSoFar = 0;
        ByteBuffer output = ByteBuffer.wrap(destination);
        
        for (long i = 0; i < totalObjects; i++) {
            ByteBuffer buf = io.read(nfsPath, objectSize, new ObjectOffset(startObject + i));
            int toBeRead = Math.min(buf.remaining() - startOffset, (objectSize - startOffset));
            buf.position(buf.position() + startOffset);
            buf.limit(buf.position() + toBeRead);
            output.put(buf);
            startOffset = 0;
            length -= toBeRead;
            readSoFar += toBeRead;
        }

        return readSoFar;
    }

    public interface ChunkIo {
        public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) throws Exception;
        public void write(NfsPath path, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) throws Exception;
    }
}
