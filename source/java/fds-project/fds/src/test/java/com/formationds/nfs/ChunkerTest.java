package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Before;
import org.junit.Test;

import java.util.HashMap;
import java.util.Random;
import java.util.UUID;

import static org.junit.Assert.*;

public class ChunkerTest {

    public static final int OBJECT_SIZE = 1024;
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    private Chunker chunker;
    private TransactionalIo transactions;
    private MemoryIoOps io;

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
        io.writeMetadata(DOMAIN, VOLUME, blobName, new HashMap<>(), false);
        chunker.write(DOMAIN, VOLUME, blobName, OBJECT_SIZE, bytes, 0, length, meta -> meta.put("key", arbitraryValue));
        transactions.mapObjectAndMetadata(DOMAIN, VOLUME, blobName, OBJECT_SIZE, new ObjectOffset(0), (oov) -> {
            assertTrue(oov.isPresent());
            assertEquals(arbitraryValue, oov.get().getMetadata().get("key"));
            return null;
        });
        byte[] readBuf = new byte[length];
        chunker.read(DOMAIN, VOLUME, blobName, OBJECT_SIZE, readBuf, 0, length);
        assertArrayEquals(bytes, readBuf);
        chunker.read(DOMAIN, VOLUME, blobName, OBJECT_SIZE, readBuf, 0, length);
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
        io = new MemoryIoOps();
        transactions = new TransactionalIo(io);
        chunker = new Chunker(transactions);
    }
}