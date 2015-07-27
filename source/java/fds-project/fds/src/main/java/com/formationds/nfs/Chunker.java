package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;

public class Chunker {
    private static final Logger LOG = Logger.getLogger(Chunker.class);

    public Chunker(ChunkIo io) {
        this.io = io;
    }

    private ChunkIo io;

    public void write(NfsEntry entry, int objectSize, byte[] bytes, long offset, int length, long blobSize) throws Exception {
        LOG.debug("Chunker write() " + entry.path() + ", objectSize=" + objectSize + ", bytes=" + bytes.length + ", offset=" + offset + ", length=" + length + ", blobSize=" + blobSize);

        length = Math.min(bytes.length, length);
        if (length == 0) {
            return;
        }

        long startObject = Math.floorDiv(offset, objectSize);
        int startOffset = (int) (offset % objectSize);
        int remaining = length;

        for (long i = startObject; remaining > 0; i++) {
            boolean isEndOfBlob = offset + length == blobSize && (remaining + startOffset <= objectSize);
            int toBeWritten = 0;
            if (isEndOfBlob) {
                toBeWritten = remaining;
            } else {
                toBeWritten = Math.min(objectSize - startOffset, remaining);
            }
            ByteBuffer writeBuf = ByteBuffer.allocate(objectSize);
            try {
                ByteBuffer readBuf = io.read(entry.path(), objectSize, new ObjectOffset(i));
                writeBuf.put(readBuf);
                writeBuf.position(0);
            } catch (FileNotFoundException e) {

            }

            writeBuf.position(startOffset);
            writeBuf.put(bytes, length - remaining, toBeWritten);
            writeBuf.position(0);

            if (isEndOfBlob) {
                writeBuf.limit(startOffset + toBeWritten);
            }

            io.write(entry, objectSize, new ObjectOffset(i), writeBuf, isEndOfBlob);
            startOffset = 0;
            remaining -= toBeWritten;
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

    public void move(NfsEntry source, NfsEntry destination, int objectSize, long blobSize) throws Exception {
        long remaining = blobSize;
        LOG.debug("Chunker: moving " + source.path() + " to " + destination.path());
        for (long i = 0; remaining > 0; i++) {
            boolean isEndOfBlob = remaining < objectSize;
            ByteBuffer buf = io.read(source.path(), objectSize, new ObjectOffset(i));
            LOG.debug("Buf: " + buf.remaining() + " bytes, blobSize=" + blobSize + ", isEndOfBlob=" + isEndOfBlob);
            io.write(destination, objectSize, new ObjectOffset(i), buf, isEndOfBlob);
            remaining -= objectSize;
        }
    }

    public interface ChunkIo {
        public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) throws Exception;

        public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, boolean isEndOfBlob) throws Exception;
    }
}
