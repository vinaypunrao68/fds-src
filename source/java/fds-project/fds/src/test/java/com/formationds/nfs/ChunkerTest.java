package com.formationds.nfs;

public class ChunkerTest {
    /*
    @Test
    public void testMove() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry source = new NfsEntry(new NfsPath("foo", "/hello/source"), null);
        NfsEntry destination = new NfsEntry(new NfsPath("foo", "/hello/destination"), null);
        int objectSize = 10;
        int blobSize = 55;
        byte[] buf = randomBytes(blobSize);
        chunker.write(source, objectSize, buf, 0, buf.length, blobSize);
        chunker.move(source, destination, objectSize, blobSize);
        byte[] readBuf = new byte[blobSize];
        chunker.read(destination.path(), objectSize, readBuf, 0, blobSize);
        assertArrayEquals(buf, readBuf);
    }

    @Test
    public void testExtend() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        int objectSize = 131072;
        int fileLength = objectSize + 1;
        chunker.write(nfsEntry, objectSize, new byte[42], 0, 42, 42);
        byte[] writeBuf = randomBytes(fileLength);
        chunker.write(nfsEntry, objectSize, writeBuf, 0, fileLength, fileLength);
        byte[] buf = new byte[fileLength];
        chunker.read(nfsEntry.path(), objectSize, buf, 0, fileLength);
        assertArrayEquals(writeBuf, buf);
    }

    @Test
    public void testWriteChunksInArbitraryOrder() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        int objectSize = 131072;
        int blockSize = 4096;
        int blockCount = 2;
        int length = blockSize * blockCount; // About 400M
        byte[] bytes = randomBytes(length);
        List<Integer> chunks = new ArrayList<>(blockCount);
        for (int i = 0; i < blockCount; i++) {
            chunks.add(blockCount - i - 1);
        }

        chunks.stream()
                .forEach(i -> {
                    byte[] chunk = new byte[blockSize];
                    System.arraycopy(bytes, i * blockSize, chunk, 0, blockSize);
                    try {
                        chunker.write(nfsEntry, objectSize, chunk, i * blockSize, blockSize, length);
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
        int length = objectSize * objectCount;
        byte[] bytes = randomBytes(length);
        chunker.write(nfsEntry, objectSize, bytes, 0, length, length);
        byte[] buf = new byte[length];
        chunker.read(nfsEntry.path(), objectSize, buf, 0, length);
        assertArrayEquals(bytes, buf);
    }

    @Test
    public void testWrite() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsPath path = new NfsPath("foo", "/hello/world");
        NfsEntry nfsEntry = new NfsEntry(path, null);
        byte[] bytes = randomBytes(6);
        chunker.write(nfsEntry, 4, bytes, 2, 6, 8);
        assertEquals(0, io.byteAt(path, 4, 0));
        assertEquals(0, io.byteAt(path, 4, 1));
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.byteAt(path, 4, i + 2));
        }
    }

    @Test
    public void testWriteMisalignedBoundaries() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsPath path = new NfsPath("foo", "/hello/world");
        NfsEntry nfsEntry = new NfsEntry(path, null);
        byte[] bytes = randomBytes(8);
        chunker.write(nfsEntry, 4, bytes, 2, 8, 10);
        for (int i = 0; i < 6; i++) {
            assertEquals(bytes[i], io.byteAt(path, 4, i + 2));
        }
    }

    @Test
    public void testReadWrite() throws Exception {
        MemoryIo io = new MemoryIo();
        Chunker chunker = new Chunker(io);
        NfsEntry nfsEntry = new NfsEntry(new NfsPath("foo", "/hello/world"), null);
        byte[] bytes = randomBytes(8);
        chunker.write(nfsEntry, 4, bytes, 2, 10, 12);
        byte[] readBuf = new byte[12];
        int read = chunker.read(nfsEntry.path(), 4, readBuf, 0, 10);
        assertEquals(12, read);
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

    private byte[] randomBytes(int length) {
        byte[] bytes = new byte[length];
        RNG.nextBytes(bytes);
        return bytes;
    }

    private static Random RNG = new Random();

    static class MemoryIo implements Chunker.ChunkIo {
        private Map<String, Map<Long, byte[]>> blobs;

        public MemoryIo() {
            blobs = new HashMap<>();
        }

        public byte byteAt(NfsPath path, int objectSize, long offset) {
            long objectId = Math.floorDiv(offset, objectSize);
            int objectOffset = (int) (offset % objectSize);
            return blobs.get(path.blobName()).get(objectId)[objectOffset];
        }

        @Override
        public ByteBuffer read(NfsPath path, int objectSize, ObjectOffset objectOffset) throws Exception {
            if (!blobs.containsKey(path.blobName())) {
                throw new FileNotFoundException();
            }

            if (!blobs.get(path.blobName()).containsKey(objectOffset.getValue())) {
                throw new FileNotFoundException(path + ", objectOffset=" + objectOffset.getValue());
            }

            return ByteBuffer.wrap(blobs.get(path.blobName()).get(objectOffset.getValue()));
        }

        @Override
        public void write(NfsEntry entry, int objectSize, ObjectOffset objectOffset, ByteBuffer byteBuffer, boolean isEndOfBlob) {
            if (byteBuffer.remaining() == 0) {
                throw new RuntimeException("Empty object!");
            }

            if (!isEndOfBlob && byteBuffer.remaining() != objectSize) {
                throw new RuntimeException("All objects except the last one should be exactly MAX_OBJECT_SIZE long");
            }

            byte[] bytes = new byte[byteBuffer.remaining()];
            byteBuffer.get(bytes);
            blobs.compute(entry.path().blobName(), (k, v) -> {
                if (v == null) {
                    v = new HashMap<>();
                }
                return v;
            }).put(objectOffset.getValue(), bytes);
        }
    }
    */
}