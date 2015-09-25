package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

public class IoCacheTest {

    private String domain;
    private String volume;
    private String blobName;
    private int objectSize;
    private Io io;
    private Io cache;

    @Test
    public void testMapAndMutateMetadata() throws Exception {
        cache.mutateMetadata(domain, volume, blobName, (meta) -> meta.put("foo", "bar"));
        assertEquals("bar", io.mapMetadata(domain, volume, blobName, om -> om.get().get("foo")));
        assertEquals("bar", cache.mapMetadata(domain, volume, blobName, om -> om.get().get("foo")));

        cache.mutateMetadata(domain, volume, blobName, meta -> meta.put("foo", "panda"));
        assertEquals("panda", io.mapMetadata(domain, volume, blobName, om -> om.get().get("foo")));
        assertEquals("panda", cache.mapMetadata(domain, volume, blobName, om -> om.get().get("foo")));
    }

    @Test
    public void testDelete() throws Exception {
        testMapAndMutateMetadata();
        cache.deleteBlob(domain, volume, blobName);

        Optional<Map<String, String>> cachedMeta = cache.mapMetadata(domain, volume, blobName, om -> om);
        assertFalse(cachedMeta.isPresent());
        Optional<ObjectView> cachedObject = cache.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov);
        assertFalse(cachedObject.isPresent());

        Optional<Map<String, String>> storedMeta = io.mapMetadata(domain, volume, blobName, om -> om);
        assertFalse(storedMeta.isPresent());
        Optional<ObjectView> storedObject = io.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov);
        assertFalse(storedObject.isPresent());
    }

    @Test
    public void testDirectMutate() throws Exception {
        Map<String, String> metadata = new HashMap<>();
        metadata.put("foo", "bar");
        ByteBuffer buffer = ByteBuffer.allocate(10);
        buffer.putInt(42);
        buffer.position(0);
        cache.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0), buffer.duplicate(), metadata);
        ByteBuffer result = io.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0), oov -> oov.get().getBuf());
        assertEquals(42, result.getInt());
    }

    @Test
    public void testMapAndMutateObjects() throws Exception {

        Map<String, String> metadata = new HashMap<>();
        metadata.put("foo", "bar");
        cache.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> new ObjectView(metadata, ByteBuffer.allocate(10)));

        ByteBuffer byteBuffer = cache.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov.get().getBuf());
        assertEquals(objectSize, byteBuffer.remaining());
        assertEquals(0, byteBuffer.get());

        // Mutate sure we get a sliced ByteBuffer
        byteBuffer = cache.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov.get().getBuf());
        assertEquals(objectSize, byteBuffer.remaining());

        cache.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                ov -> {
                    ByteBuffer buf = ov.getBuf();
                    buf.putInt(42);
                    buf.position(0);
                });

        // Mutate sure we get a sliced ByteBuffer
        byteBuffer = cache.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov.get().getBuf());
        assertEquals(objectSize, byteBuffer.remaining());
        assertEquals(42, byteBuffer.getInt());
    }

    @Before
    public void setUp() throws Exception {
        domain = UUID.randomUUID().toString();
        volume = UUID.randomUUID().toString();
        blobName = UUID.randomUUID().toString();
        objectSize = 42;
        io = new MemoryIo();
        cache = new IoCache(io, new Counters());
    }
}