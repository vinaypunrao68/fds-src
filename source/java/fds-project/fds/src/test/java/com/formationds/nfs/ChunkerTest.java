package com.formationds.nfs;

import org.junit.Before;
import org.junit.Test;

import java.util.*;

import static org.junit.Assert.*;

public class ChunkerTest {

    public static final int MAX_OBJECT_SIZE = 1024;
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    private Chunker chunker;
    private IoOps io;

    @Test
    public void testReadWrite() throws Exception {
        readWriteCycle(42);
        readWriteCycle(84);
        readWriteCycle(1025);
        readWriteCycle(1024);
        readWriteCycle(2049);
    }

    private void readWriteCycle(int length) throws Exception {
        byte[] bytes = randomBytes(length);
        String arbitraryValue = UUID.randomUUID().toString();
        String blobName = "blobName";
        io.writeMetadata(DOMAIN, VOLUME, blobName, new HashMap<String, String>(new HashMap<>()));
        io.commitMetadata(DOMAIN, VOLUME, blobName);
        chunker.write(DOMAIN, VOLUME, blobName, MAX_OBJECT_SIZE, bytes, 0, length, meta -> meta.put("key", arbitraryValue));
        Optional<Map<String, String>> ofm = io.readMetadata(DOMAIN, VOLUME, blobName);
        assertTrue(ofm.isPresent());
        assertEquals(arbitraryValue, ofm.get().get("key"));
        byte[] readBuf = new byte[length];
        chunker.read(DOMAIN, VOLUME, blobName, MAX_OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
        chunker.read(DOMAIN, VOLUME, blobName, MAX_OBJECT_SIZE, readBuf, 0, length);
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
    }
}