package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.ByteBufferUtility;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.*;

public class DeferredIoOpsTest {

    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB = "blob/";
    public static final int OBJECT_SIZE = 42;
    public static final int MAX_OBJECT_SIZE = 1024;
    private MemoryIoOps backend;
    private DeferredIoOps deferredIo;

    @Test
    public void testCommit() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        FdsObject object = new FdsObject(ByteBuffer.allocate(MAX_OBJECT_SIZE), MAX_OBJECT_SIZE);
        AtomicInteger commitCount = new AtomicInteger(0);
        deferredIo.addCommitListener(key -> commitCount.incrementAndGet());

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, metadata);
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), object);
        assertFalse(backend.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertEquals(0, backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), MAX_OBJECT_SIZE).limit());
        assertEquals(0, commitCount.get());

        deferredIo.commitMetadata(DOMAIN, VOLUME, BLOB);
        assertTrue(backend.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertEquals(1, commitCount.get());

        deferredIo.commitObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0));
        assertEquals(MAX_OBJECT_SIZE, backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), MAX_OBJECT_SIZE).limit());
    }

    @Test
    public void testBlobDeletesAreDeferred() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        FdsObject fdsObject = new FdsObject(ByteBuffer.allocate(1024), 1024);

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(metadata));
        deferredIo.commitMetadata(DOMAIN, VOLUME, BLOB);
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), fdsObject);
        assertTrue(backend.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertTrue(backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), MAX_OBJECT_SIZE) != null);

        deferredIo.deleteBlob(DOMAIN, VOLUME, BLOB);
        assertTrue(backend.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertTrue(backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), MAX_OBJECT_SIZE) != null);
        assertFalse(deferredIo.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());

        deferredIo.commitMetadata(DOMAIN, VOLUME, BLOB);
        assertFalse(backend.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertFalse(deferredIo.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
    }

    @Test
    public void testDeletedBlobsDontShowUpInScanResults() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("hello", "world");
        String blobName = "foo/bar";
        deferredIo.writeMetadata(DOMAIN, VOLUME, blobName, new HashMap<>(metadata));
        deferredIo.commitMetadata(DOMAIN, VOLUME, blobName);
        assertEquals(1, deferredIo.scan(DOMAIN, VOLUME, "foo/").size());
        deferredIo.deleteBlob(DOMAIN, VOLUME, blobName);
        assertEquals(0, deferredIo.scan(DOMAIN, VOLUME, "foo/").size());
    }

    @Test
    public void testVolumeDelete() throws Exception {
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>());
        deferredIo.writeMetadata(DOMAIN, "wolume", BLOB, new HashMap<>());
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), new FdsObject(ByteBuffer.allocate(10), OBJECT_SIZE));
        assertTrue(deferredIo.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        deferredIo.onVolumeDeletion(DOMAIN, VOLUME);
        assertFalse(deferredIo.readMetadata(DOMAIN, VOLUME, BLOB).isPresent());
        assertTrue(deferredIo.readMetadata(DOMAIN, "wolume", BLOB).isPresent());

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>());
        assertEquals(0, deferredIo.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE).limit());
    }

    @Test
    public void testImmediateObjectWrites() throws Exception {
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>());
        deferredIo.commitMetadata(DOMAIN, VOLUME, BLOB);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), FdsObject.wrap(bytes, OBJECT_SIZE));
        deferredIo.commitObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0));
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
        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(map));
        deferredIo.commitMetadata(DOMAIN, VOLUME, BLOB);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        deferredIo.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), FdsObject.wrap(bytes, OBJECT_SIZE));
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
        backend.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(storedMetadata));
        backend.commitMetadata(DOMAIN, VOLUME, BLOB);

        Map<String, String> cachedMetadata = new HashMap<>();
        cachedMetadata.put("value", "new");

        deferredIo.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(cachedMetadata));

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, BLOB);
        assertEquals(1, results.size());
        assertEquals("new", results.iterator().next().getMetadata().get("value"));
    }

    @Test
    public void testScanReturnsBothStoredAndCachedEntries() throws Exception {
        backend.writeMetadata(DOMAIN, VOLUME, "b/a", new HashMap<>());
        backend.writeMetadata(DOMAIN, VOLUME, "b/c", new HashMap<>());

        deferredIo.writeMetadata(DOMAIN, VOLUME, "a/a", new HashMap<>());
        deferredIo.writeMetadata(DOMAIN, VOLUME, "b/b", new HashMap<>());
        deferredIo.writeMetadata(DOMAIN, VOLUME, "c/a", new HashMap<>());

        Collection<BlobMetadata> results = deferredIo.scan(DOMAIN, VOLUME, "b/");
        assertEquals(3, results.size());
    }

    @Before
    public void setUp() throws Exception {
        backend = new MemoryIoOps(MAX_OBJECT_SIZE);
        deferredIo = new DeferredIoOps(backend, v -> MAX_OBJECT_SIZE);
    }
}