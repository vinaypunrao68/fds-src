package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.Assert.*;

public class ChunkerTest {

    public static final int MAX_OBJECT_SIZE = 1024;
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB_NAME = "blobName";
    private Chunker chunker;
    private CountingIo io;

    @Test
    public void testNoReadsForAlignedWrites() throws Exception {
        byte[] buf = new byte[MAX_OBJECT_SIZE];
        new Random().nextBytes(buf);
        chunker.write(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, buf, 0, MAX_OBJECT_SIZE, x -> null);
        assertEquals(0, io.objectReads.get());
        assertEquals(1, io.objectWrites.get());
        byte[] dest = new byte[MAX_OBJECT_SIZE];
        int read = chunker.read(DOMAIN, VOLUME, BLOB_NAME, MAX_OBJECT_SIZE, dest, 0, MAX_OBJECT_SIZE);
        assertEquals(MAX_OBJECT_SIZE, read);
        assertArrayEquals(buf, dest);
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
        io = new CountingIo(MAX_OBJECT_SIZE);
        chunker = new Chunker(io);
        io.writeMetadata(DOMAIN, VOLUME, BLOB_NAME, new HashMap<>(new HashMap<>()));
        io.commitMetadata(DOMAIN, VOLUME, BLOB_NAME);
    }

    private static class CountingIo extends MemoryIoOps {
        final AtomicLong objectReads;
        final AtomicLong objectWrites;

        public CountingIo(int maxObjectSize) {
            super(maxObjectSize);
            objectReads = new AtomicLong(0);
            objectWrites = new AtomicLong(0);
        }

        @Override
        public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
            objectReads.incrementAndGet();
            return super.readCompleteObject(domain, volumeName, blobName, objectOffset, maxObjectSize);
        }

        @Override
        public void writeObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, FdsObject fdsObject) throws IOException {
            objectWrites.incrementAndGet();
            super.writeObject(domain, volumeName, blobName, objectOffset, fdsObject);
        }
    }

}
