package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.util.ByteBufferUtility;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class DeferredIoOpsTest {
    @Test
    public void testRenameBlob() throws Exception {
        MemoryIoOps backend = new MemoryIoOps();
        IoOps io = new DeferredIoOps(backend, new Counters());
        Map<String, String> map = new HashMap<>();
        map.put("hello", "world");
        io.writeMetadata("domain", "volume", "blob", map, false);
        byte[] bytes = ByteBufferUtility.randomBytes(10).array();
        io.writeObject("domain", "volume", "blob", new ObjectOffset(0), ByteBuffer.wrap(bytes), bytes.length, true);
    }
}