package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.ByteBufferUtility;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class DeferredIoOpsTest {

    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB = "blob/";
    public static final int OBJECT_SIZE = 42;
    private MemoryIoOps backend;
    private IoOps deferredIo;

    @Test
    public void testImmediateObjectWrites() throws Exception {
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(), false);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), ByteBuffer.wrap(bytes), OBJECT_SIZE, false);
        ByteBuffer read = backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE);
        assertArrayEquals(bytes, read.array());
    }

    @Test
    public void testRenameBlob() throws Exception {
        Map<String, String> map = new HashMap<>();
        map.put("hello", "world");
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, map, false);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), ByteBuffer.wrap(bytes), bytes.length, true);
        assertArrayEquals(bytes, deferredIo.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE).array());

        deferredIo.renameBlob(DOMAIN, VOLUME, BLOB, "schmoops");
        assertArrayEquals(bytes, backend.readCompleteObject(DOMAIN, VOLUME, "schmoops", new ObjectOffset(0), OBJECT_SIZE).array());
        assertArrayEquals(bytes, backend.readCompleteObject(DOMAIN, VOLUME, "schmoops", new ObjectOffset(0), OBJECT_SIZE).array());
    }

    @Test
    public void testScanReturnsFreshEntries() throws Exception {
        Map<String, String> storedMetadata = new HashMap<>();
        storedMetadata.put("value", "old");
        backend.writeMetadata(DOMAIN, VOLUME, BLOB, storedMetadata, false);

        Map<String, String> cachedMetadata = new HashMap<>();
        cachedMetadata.put("value", "new");

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, cachedMetadata, true);

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, BLOB);
        assertEquals(1, results.size());
        assertEquals("new", results.iterator().next().getMetadata().get("value"));
    }

    @Test
    public void testScanReturnsBothStoredAndCachedEntries() throws Exception {
        backend.writeMetadata(DOMAIN, VOLUME, "b/a", new HashMap<>(), true);
        backend.writeMetadata(DOMAIN, VOLUME, "b/c", new HashMap<>(), true);

        deferredIo.writeMetadata(DOMAIN, VOLUME, "a/a", new HashMap<>(), true);
        deferredIo.writeMetadata(DOMAIN, VOLUME, "b/b", new HashMap<>(), true);
        deferredIo.writeMetadata(DOMAIN, VOLUME, "c/a", new HashMap<>(), true);

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, "b/");
        assertEquals(3, results.size());
    }

    @Before
    public void setUp() throws Exception {
        backend = new MemoryIoOps();
        deferredIo = new DeferredIoOps(backend, new Counters());
    }
}