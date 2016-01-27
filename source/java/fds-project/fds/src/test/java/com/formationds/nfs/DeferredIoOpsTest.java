package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.ByteBufferUtility;
import org.junit.Before;
import org.junit.Test;

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
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new FdsMetadata(), false);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), FdsObject.wrap(bytes, OBJECT_SIZE), false);
        FdsObject fdsObject = backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE);
        fdsObject.lock(o -> {
            assertArrayEquals(bytes, o.toByteArray());
            return null;
        });
    }

    @Test
    public void testRenameBlob() throws Exception {
        Map<String, String> map = new HashMap<>();
        map.put("hello", "world");
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new FdsMetadata(map), false);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), FdsObject.wrap(bytes, OBJECT_SIZE), true);
        deferredIo.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE).lock(o -> {
            assertArrayEquals(bytes, o.toByteArray());
            return null;
        });

        deferredIo.renameBlob(DOMAIN, VOLUME, BLOB, "schmoops");
        backend.readCompleteObject(DOMAIN, VOLUME, "schmoops", new ObjectOffset(0), OBJECT_SIZE).lock(o -> {
            assertArrayEquals(bytes, o.toByteArray());
            return null;
        });
        backend.readCompleteObject(DOMAIN, VOLUME, "schmoops", new ObjectOffset(0), OBJECT_SIZE).lock(o -> {
            assertArrayEquals(bytes, o.toByteArray());
            return null;
        });
    }

    @Test
    public void testScanReturnsFreshEntries() throws Exception {
        Map<String, String> storedMetadata = new HashMap<>();
        storedMetadata.put("value", "old");
        backend.writeMetadata(DOMAIN, VOLUME, BLOB, new FdsMetadata(storedMetadata), false);

        Map<String, String> cachedMetadata = new HashMap<>();
        cachedMetadata.put("value", "new");

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new FdsMetadata(cachedMetadata), true);

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, BLOB);
        assertEquals(1, results.size());
        assertEquals("new", results.iterator().next().getMetadata().lock(m -> m.mutableMap().get("value")));
    }

    @Test
    public void testScanReturnsBothStoredAndCachedEntries() throws Exception {
        backend.writeMetadata(DOMAIN, VOLUME, "b/a", new FdsMetadata(), true);
        backend.writeMetadata(DOMAIN, VOLUME, "b/c", new FdsMetadata(), true);

        deferredIo.writeMetadata(DOMAIN, VOLUME, "a/a", new FdsMetadata(), true);
        deferredIo.writeMetadata(DOMAIN, VOLUME, "b/b", new FdsMetadata(), true);
        deferredIo.writeMetadata(DOMAIN, VOLUME, "c/a", new FdsMetadata(), true);

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, "b/");
        assertEquals(3, results.size());
    }

    @Before
    public void setUp() throws Exception {
        backend = new MemoryIoOps();
        deferredIo = new DeferredIoOps(backend, new Counters());
    }
}