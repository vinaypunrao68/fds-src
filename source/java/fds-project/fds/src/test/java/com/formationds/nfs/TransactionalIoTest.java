package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import org.junit.Before;
import org.junit.Test;

import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.UUID;

import static org.junit.Assert.*;

public class TransactionalIoTest {
    private String domain;
    private String volume;
    private String blobName;
    private int objectSize;
    private IoOps ioOps;
    private TransactionalIo transactionalIo;

    @Test
    public void testRename() throws Exception {
        transactionalIo.mutateMetadata(domain, volume, blobName, false, (meta) -> meta.put("foo", "bar"));
        assertEquals("bar", ioOps.readMetadata(domain, volume, blobName).get().get("foo"));

        String newName = "ploop";
        transactionalIo.renameBlob(domain, volume, blobName, newName);
        assertEquals("bar", ioOps.readMetadata(domain, volume, newName).get().get("foo"));
    }

    @Test
    public void testMapAndMutateMetadata() throws Exception {
        transactionalIo.mutateMetadata(domain, volume, blobName, false, (meta) -> meta.put("foo", "bar"));
        assertEquals("bar", ioOps.readMetadata(domain, volume, blobName).get().get("foo"));
        assertEquals("bar", transactionalIo.mapMetadata(domain, volume, blobName, (x, om) -> om.get().get("foo")));

        transactionalIo.mutateMetadata(domain, volume, blobName, false, meta -> meta.put("foo", "panda"));
        assertEquals("panda", ioOps.readMetadata(domain, volume, blobName).get().get("foo"));
        assertEquals("panda", transactionalIo.mapMetadata(domain, volume, blobName, (x, om) -> om.get().get("foo")));
    }

    @Test
    public void testDelete() throws Exception {
        testMapAndMutateMetadata();
        transactionalIo.deleteBlob(domain, volume, blobName);

        Optional<Map<String, String>> cachedMeta = transactionalIo.mapMetadata(domain, volume, blobName, (x, om) -> om);
        assertFalse(cachedMeta.isPresent());
        Optional<ObjectAndMetadata> cachedObject = transactionalIo.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov);
        assertFalse(cachedObject.isPresent());

        Optional<Map<String, String>> storedMeta = ioOps.readMetadata(domain, volume, blobName);
        assertFalse(storedMeta.isPresent());
        try {
            ioOps.readCompleteObject(domain, volume, blobName, new ObjectOffset(0), objectSize);
        } catch (FileNotFoundException e) {
            return;
        }
        fail("Should have gotten a FileNotFoundException!");
    }

    @Test
    public void testMapAndMutateObjects() throws Exception {

        Map<String, String> metadata = new HashMap<>();
        metadata.put("foo", "bar");
        transactionalIo.mutateMetadata(domain, volume, blobName, new HashMap<>(), false);
        transactionalIo.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                false, oov -> new ObjectAndMetadata(metadata, ByteBuffer.allocate(10)));

        ByteBuffer byteBuffer = transactionalIo.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov.get().getBuf());
        assertEquals(objectSize, byteBuffer.remaining());
        assertEquals(0, byteBuffer.get());

        // Mutate sure we get a sliced ByteBuffer
        byteBuffer = transactionalIo.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
                oov -> oov.get().getBuf());
        assertEquals(objectSize, byteBuffer.remaining());

        transactionalIo.mutateObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0), false,
                ov -> {
                    ByteBuffer buf = ov.getBuf();
                    buf.putInt(42);
                    buf.position(0);
                });

        // Mutate sure we get a sliced ByteBuffer
        byteBuffer = transactionalIo.mapObjectAndMetadata(domain, volume, blobName, objectSize, new ObjectOffset(0),
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
        ioOps = new MemoryIoOps();
        transactionalIo = new TransactionalIo(new DeferredIoOps(ioOps, new Counters()));
    }
}