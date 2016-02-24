package com.formationds.nfs;

import org.junit.Before;
import org.junit.Test;

import java.util.*;

import static org.junit.Assert.*;

public class ChunkerTest {

    public static final int MAX_OBJECT_SIZE = 1024;
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB_NAME = "blobName";
    private Chunker chunker;
    private IoOps io;

    @Test
    public void testIntFun() throws Exception {
        long offset = Long.parseLong("90696900608");
        int objectSize = 1024 * 1024 * 2;
        int startOffset = (int) (offset % objectSize);
        assertTrue(startOffset > 0);
        assertTrue(startOffset < objectSize);
    }

    @Test
    public void testReadWrite() throws Exception {
        readWriteCycle(42);
        readWriteCycle(84);
        readWriteCycle(1025);
        readWriteCycle(1024);
        readWriteCycle(2049);
    }

    @Test
    public void testReadPastEnd() throws Exception {
        int length = MAX_OBJECT_SIZE * 2;
        byte[] buf = new byte[length];
        new Random().nextBytes(buf);
        chunker.write(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, buf, 0, length, x -> null);
        byte[] dest = new byte[10];
        int read = chunker.read(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, dest, MAX_OBJECT_SIZE * 2, 10);
        assertEquals(0, read);
    }

    @Test
    public void testUnalignedReads() throws Exception {
        int length = MAX_OBJECT_SIZE * 2;
        byte[] buf = new byte[length];
        new Random().nextBytes(buf);
        chunker.write(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, buf, 0, length, x -> null);
        byte[] dest = new byte[10];
        int read = chunker.read(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, dest, MAX_OBJECT_SIZE - 5, 10);
        assertEquals(10, read);
    }

    private void readWriteCycle(int length) throws Exception {
        byte[] bytes = randomBytes(length);
        String arbitraryValue = UUID.randomUUID().toString();
        chunker.write(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, bytes, 0, length, meta -> meta.put("key", arbitraryValue));
        Optional<Map<String, String>> ofm = io.readMetadata(DOMAIN, VOLUME, BLOB_NAME);
        assertTrue(ofm.isPresent());
        assertEquals(arbitraryValue, ofm.get().get("key"));
        byte[] readBuf = new byte[length];
        chunker.read(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
        chunker.read(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
    }

    private byte[] randomBytes(int length) {
        byte[] bytes = new byte[length];
        RNG.nextBytes(bytes);
        return bytes;
    }

    private static Random RNG = new Random();

    @Before
    public void setUp() throws Exception {
        io = new MemoryIoOps(MAX_OBJECT_SIZE);
        chunker = new Chunker(io);
        io.writeMetadata(DOMAIN, VOLUME, BLOB_NAME, new HashMap<>(new HashMap<>()));
        io.commitMetadata(DOMAIN, VOLUME, BLOB_NAME);
    }
}