package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Test;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.*;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class ChunkerTest {
    @Test
    public void testWriteChunksInArbitraryOrder() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        int objectSize = 131072;
        int blockSize = 4096;
        int blockCount = 2;
        int length = blockSize * blockCount; // About 400M
        byte[] bytes = new byte[length];
        new Random().nextBytes(bytes);
        List<Integer> chunks = new ArrayList<>(blockCount);
        for (int i = 0; i < blockCount; i++) {
            chunks.add(blockCount - i - 1);
        }

        chunks.stream()
                .forEach(i -> {
                    byte[] chunk = new byte[blockSize];
                    System.arraycopy(bytes, i * blockSize, chunk, 0, blockSize);
                    try {
                        chunker.write(nfsEntry, objectSize, chunk, i * blockSize, blockSize);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                });
        byte[] readBuf = new byte[length];
        int read = chunker.read(nfsEntry.path(), objectSize, readBuf, 0, length);
        assertEquals(length, read);
        assertArrayEquals(bytes, readBuf);
    }

    @Test
    public void testLargeChunks() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        int objectSize = 131072;
        int objectCount = 11;
        byte[] bytes = new byte[objectSize * objectCount];
        new Random().nextBytes(bytes);
        chunker.write(nfsEntry, objectSize, bytes, 0, objectSize * objectCount);
        byte[] buf = new byte[objectSize * objectCount];
        chunker.read(nfsEntry.path(), objectSize, buf, 0, objectSize * objectCount);
        assertArrayEquals(bytes, buf);
    }

    @Test
    public void testWrite() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        byte[] bytes = new byte[6];
        new Random().nextBytes(bytes);
        chunker.write(nfsEntry, 4, bytes, 2, 6);
        assertEquals(0, io.byteAt(4, 0));
        assertEquals(0, io.byteAt(4, 1));
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.byteAt(4, i + 2));
        }
    }

    @Test
    public void testWriteMisalignedBoundaries() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        byte[] bytes = new byte[8];
        new Random().nextBytes(bytes);
        chunker.write(nfsEntry, 4, bytes, 2, 8);
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.byteAt(4, i + 2));
        }
    }

    @Test
    public void testReadWrite() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        byte[] bytes = new byte[8];
        new Random().nextBytes(bytes);
        chunker.write(nfsEntry, 4, bytes, 2, 10);
        byte[] readBuf = new byte[12];
        int read = chunker.read(nfsEntry.path(), 4, readBuf, 0, 10);
        assertEquals(10, read);
        assertEquals(0, readBuf[0]);
        assertEquals(0, readBuf[1]);
        for (int i = 0; i < 8; i++) {
            assertEquals(bytes[i], readBuf[i + 2]);
        }

        byte[] smallerBuf = new byte[8];
        read = chunker.read(nfsEntry.path(), 4, smallerBuf, 2, 10);
        assertEquals(8, read);
        for (int i = 0; i < 8; i++) {
            assertEquals(bytes[i], smallerBuf[i]);
        }

        byte[] tinyBuf = new byte[3];
        read = chunker.read(nfsEntry.path(), 4, tinyBuf, 2, 10);
        assertEquals(3, read);
        for (int i = 0; i < 3; i++) {
            assertEquals(bytes[i], tinyBuf[i]);
        }
    }

    static class MemoryIo implements Chunker.ChunkIo {
        private Map<Long, byte[]> objects;

        public MemoryIo() {
            objects = new HashMap<>();
        }

        public byte byteAt(int objectSize, long offset) {
            long objectId = Math.floorDiv(offset, objectSize);
            int objectOffset = (int) (offset % objectSize);
            return objects.get(objectId)[objectOffset];
        }

        @Override
        public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) throws Exception {
            if (!objects.containsKey(objectOffset.getValue())) {
                throw new FileNotFoundException(path + ", objectOffset=" + objectOffset.getValue());
            }

            return ByteBuffer.wrap(objects.get(objectOffset.getValue()));
        }

        @Override
        public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer) {
            if (byteBuffer.remaining() == 0) {
                throw new RuntimeException("WTF");
            }
            byte[] bytes = new byte[byteBuffer.remaining()];
            byteBuffer.get(bytes);
            objects.put(objectOffset.getValue(), bytes);
        }
    }
}