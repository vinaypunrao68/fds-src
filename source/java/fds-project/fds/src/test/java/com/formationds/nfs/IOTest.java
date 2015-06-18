package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Test;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.Random;

import static org.junit.Assert.*;

public class IOTest {
    @Test
    public void testWrite() throws Exception {
        MemoryIo io = new MemoryIo(1000);
        Chunker chunker = new Chunker(io);
        NfsPath nfsPath = new NfsPath("foo", "/hello/world");
        byte[] bytes = new byte[6];
        new Random().nextBytes(bytes);
        chunker.write(nfsPath, 4, bytes, 2, 6);
        assertEquals(0, io.bytes[0]);
        assertEquals(0, io.bytes[1]);
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.bytes[i + 2]);
        }
    }

    @Test
    public void testWriteMisalignedBoundaries() throws Exception {
        MemoryIo io = new MemoryIo(1000);
        Chunker chunker = new Chunker(io);
        NfsPath nfsPath = new NfsPath("foo", "/hello/world");
        byte[] bytes = new byte[8];
        new Random().nextBytes(bytes);
        chunker.write(nfsPath, 4, bytes, 2, 8);
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.bytes[i + 2]);
        }
    }

    @Test
    public void testReadWrite() throws Exception {
        MemoryIo io = new MemoryIo(1000);
        Chunker chunker = new Chunker(io);
        NfsPath nfsPath = new NfsPath("foo", "/hello/world");
        byte[] bytes = new byte[8];
        new Random().nextBytes(bytes);
        chunker.write(nfsPath, 4, bytes, 2, 8);
        byte[] readBuf = new byte[12];
        int read = chunker.read(nfsPath, 4, readBuf, 0, 12);
        assertEquals(12, read);
        assertEquals(0, readBuf[0]);
        assertEquals(0, readBuf[1]);
        assertEquals(bytes[0], readBuf[2]);
        assertEquals(bytes[1], readBuf[3]);
        assertEquals(bytes[2], readBuf[4]);
        assertEquals(bytes[3], readBuf[5]);
        assertEquals(bytes[4], readBuf[6]);
        assertEquals(bytes[5], readBuf[7]);
        assertEquals(bytes[6], readBuf[8]);
        assertEquals(bytes[7], readBuf[9]);
        assertEquals(0, readBuf[10]);
        assertEquals(0, readBuf[11]);
    }

    static class MemoryIo implements Chunker.ChunkIo {
        public byte[] bytes;
        private long currentLength;

        public MemoryIo(int length) {
            bytes = new byte[length];
            currentLength = 0;
        }

        @Override
        public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) {
            return ByteBuffer.wrap(bytes, (int) (objectOffset.getValue() * objectSize), objectSize);
        }

        @Override
        public void write(NfsPath path, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) {
            ByteBuffer local = ByteBuffer.wrap(bytes, (int) (objectOffset.getValue() * objectSize), objectSize);
            ByteBuffer copy = byteBuffer.duplicate();
            if (copy.limit() > objectSize) {
                copy.limit(objectSize);
            }
            local.put(copy);
        }
    }
}