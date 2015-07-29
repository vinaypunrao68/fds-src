package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.apache.log4j.Logger;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.Map;

public class Chunker {
    private static final Logger LOG = Logger.getLogger(Chunker.class);

    public Chunker(ChunkyIo io) {
        this.io = io;
    }

    private ChunkyIo io;

    public void write(String domain, String volume, String blobName, int objectSize, byte[] bytes, long offset, int length, long blobSize, Map<String, String> metadata) throws Exception {
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
                ByteBuffer readBuf = io.read(domain, volume, blobName, objectSize, new ObjectOffset(i));
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

            io.write(domain, volume, blobName, objectSize, new ObjectOffset(i), writeBuf, isEndOfBlob, metadata);
            startOffset = 0;
            remaining -= toBeWritten;
        }
    }

    public int read(String domain, String volume, String blobName, int objectSize, byte[] destination, long offset, int length) throws Exception {
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
                buf = io.read(domain, volume, blobName, objectSize, new ObjectOffset(startObject + i));
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

    public void move(String domain, String volume, String sourceBlob, String destBlob, int objectSize, long blobSize, Map<String, String> metadata) throws Exception {
        long remaining = blobSize;
        for (long i = 0; remaining > 0; i++) {
            boolean isEndOfBlob = remaining < objectSize;
            ByteBuffer buf = io.read(domain, volume, sourceBlob, objectSize, new ObjectOffset(i));
            LOG.debug("Buf: " + buf.remaining() + " bytes, blobSize=" + blobSize + ", isEndOfBlob=" + isEndOfBlob);
            io.write(domain, volume, destBlob, objectSize, new ObjectOffset(i), buf, isEndOfBlob, metadata);
            remaining -= objectSize;
        }
    }

    public interface ChunkyIo {
        public ByteBuffer read(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset) throws Exception;

        public void write(String domain, String volume, String blobName, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, boolean isEndOfBlob, Map<String, String> metadata) throws Exception;
    }
}
