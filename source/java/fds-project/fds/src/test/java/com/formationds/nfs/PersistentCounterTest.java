package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class PersistentCounterTest {
    @Test
    public void testAllocate() throws Exception {
        IoOps io = new MemoryIoOps();
        String counterName = "foo";
        long startValue = 42;
        String panda = "panda";
        String ostrich = "ostrich";
        PersistentCounter allocator = new PersistentCounter(io, "", counterName, startValue, false);
        assertEquals(42, allocator.currentValue(panda));
        assertEquals(42, allocator.currentValue(ostrich));
        assertEquals(43, allocator.increment(panda));
        assertEquals(43, allocator.increment(ostrich));

        // test restart
        allocator = new PersistentCounter(io, "", counterName, startValue, false);
        assertEquals(143, allocator.increment(ostrich, 100));
        assertEquals(143, allocator.increment(panda, 100));

        assertEquals(142, allocator.decrement(ostrich, 1));
        assertEquals(143, allocator.currentValue(panda));
    }
}