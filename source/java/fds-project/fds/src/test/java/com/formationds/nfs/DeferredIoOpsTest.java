package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.ByteBufferUtility;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertArrayEquals;

public class DeferredIoOpsTest {

    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB = "blob";
    public static final int OBJECT_SIZE = 42;

    @Test
    public void testImmediateObjectWrites() throws Exception {
        MemoryIoOps backend = new MemoryIoOps();
        IoOps io = new DeferredIoOps(backend, new Counters());
        io.writeMetadata(DOMAIN, VOLUME, BLOB, new HashMap<>(), false);
        byte[] bytes = ByteBufferUtility.randomBytes(OBJECT_SIZE).array();
        io.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), ByteBuffer.wrap(bytes), OBJECT_SIZE, false);
        ByteBuffer read = backend.readCompleteObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), OBJECT_SIZE);
        assertArrayEquals(bytes, read.array());
    }

    @Test
    public void testRenameBlob() throws Exception {
        MemoryIoOps backend = new MemoryIoOps();
        IoOps io = new DeferredIoOps(backend, new Counters());
        Map<String, String> map = new HashMap<>();
        map.put("hello", "world");
        io.writeMetadata(DOMAIN, VOLUME, BLOB, map, false);
        byte[] bytes = ByteBufferUtility.randomBytes(10).array();
        io.writeObject(DOMAIN, VOLUME, BLOB, new ObjectOffset(0), ByteBuffer.wrap(bytes), bytes.length, true);
    }
}