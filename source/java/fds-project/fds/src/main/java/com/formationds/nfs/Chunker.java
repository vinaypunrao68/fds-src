package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;

public class Chunker {
    private ChunkIo io;

    public Chunker(ChunkIo io) {
        this.io = io;
    }

    public synchronized void write(NfsEntry entry, int objectSize, byte[] bytes, long offset, int length) throws Exception {
        length = Math.min(bytes.length, length);
        if (length == 0) {
            return;
        }

        long totalObjects = (long) Math.ceil((double) (length + (offset % objectSize)) / (double) objectSize);
        long startObject = Math.floorDiv(offset, objectSize);
        int startOffset = (int) (offset % objectSize);
        int writtenSoFar = 0;

        for (long i = 0; i < totalObjects; i++) {
            int toBeWritten = Math.min(length, (objectSize - startOffset));
            if (toBeWritten == 0) {
                return;
            }
            ByteBuffer newChunk = null;
            // No need to read the existing object if we're writing an entire one
            if (toBeWritten == objectSize && startOffset == 0) {
                newChunk = ByteBuffer.allocate(objectSize);
            } else {
                try {
                    ByteBuffer existing = io.read(entry.path(), objectSize, new ObjectOffset(startObject + i));
                    newChunk = ByteBuffer.allocate(Math.max(existing.remaining(), startOffset + toBeWritten));
                    newChunk.put(existing);
                    newChunk.position(0);

                } catch (FileNotFoundException e) {
                    newChunk = ByteBuffer.allocate(startOffset + toBeWritten);
                }
            }

            newChunk.position(startOffset);
            newChunk.put(bytes, writtenSoFar, toBeWritten);
            newChunk.position(0);
            //newChunk.limit(startOffset + toBeWritten);
            io.write(entry, objectSize, new ObjectOffset(startObject + i), newChunk);
            startOffset = 0;
            length -= toBeWritten;
            writtenSoFar += toBeWritten;
        }
    }

    public int read(NfsPath nfsPath, int objectSize, byte[] destination, long offset, int length) throws Exception {
        length = Math.min(destination.length, length);
        if (length == 0) {
            return 0;
        }

        long totalObjects = (long) Math.ceil((double) (length + (offset % objectSize)) / (double) objectSize);
        long startObject = Math.floorDiv(offset, objectSize);
        int startOffset = (int) (offset % objectSize);
        int readSoFar = 0;
        ByteBuffer output = ByteBuffer.wrap(destination);
        
        for (long i = 0; i < totalObjects; i++) {
            ByteBuffer buf = null;
            try {
                buf = io.read(nfsPath, objectSize, new ObjectOffset(startObject + i));
            } catch (FileNotFoundException e) {
                break;
            }
            int toBeRead = Math.min(buf.remaining() - startOffset, (objectSize - startOffset));
            toBeRead = Math.min(toBeRead, output.remaining());
            try {
                buf.position(buf.position() + startOffset);
            } catch (Exception e) {
                throw e;
            }
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

        public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) throws Exception;
    }
}
