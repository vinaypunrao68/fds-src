package com.formationds.nfs;

import org.junit.Test;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;

public class PersistentCounterTest {

    public static final int MAX_OBJECT_SIZE = 1024;

    @Test
    public void testAccept() throws Exception {
        AtomicInteger commits = new AtomicInteger();
        IoOps io = new MemoryIoOps(MAX_OBJECT_SIZE) {
            @Override
            public void commitMetadata(String domain, String volumeName, String blobName) throws IOException {
                commits.incrementAndGet();
            }
        };

        PersistentCounter counter = new PersistentCounter(io, "domain", "counterName", 0);
        counter.increment("volume");
        counter.accept(new MetaKey("domain", "volume", "someBlob"));
        assertEquals(1, commits.get());
        counter.accept(new MetaKey("domain", "volume", "counterName"));
        assertEquals(1, commits.get());
    }

    @Test
    public void testAllocate() throws Exception {
        IoOps io = new MemoryIoOps(MAX_OBJECT_SIZE);
        String counterName = "foo";
        long startValue = 42;
        String panda = "panda";
        String ostrich = "ostrich";
        PersistentCounter allocator = new PersistentCounter(io, "", counterName, startValue);
        assertEquals(42, allocator.currentValue(panda));
        assertEquals(42, allocator.currentValue(ostrich));
        assertEquals(43, allocator.increment(panda));
        assertEquals(43, allocator.increment(ostrich));

        // test restart
        allocator = new PersistentCounter(io, "", counterName, startValue);
        assertEquals(143, allocator.increment(ostrich, 100));
        assertEquals(143, allocator.increment(panda, 100));

        assertEquals(142, allocator.decrement(ostrich, 1));
        assertEquals(143, allocator.currentValue(panda));
    }
}