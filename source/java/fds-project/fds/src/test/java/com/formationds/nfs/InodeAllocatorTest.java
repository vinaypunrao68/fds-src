package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class InodeAllocatorTest {
    @Test
    public void testAllocate() throws Exception {
        MemoryIo io = new MemoryIo();
        InodeAllocator allocator = new InodeAllocator(io);
        long first = allocator.allocate("foo");
        long second = allocator.allocate("foo");
        assertTrue(second > first);

        // test restart
        allocator = new InodeAllocator(io);
        long third = allocator.allocate("foo");
        assertTrue(third > second);
    }
}