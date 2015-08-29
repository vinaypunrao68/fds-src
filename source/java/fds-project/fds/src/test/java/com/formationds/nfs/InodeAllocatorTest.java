package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class InodeAllocatorTest {
    @Test
    public void testAllocate() throws Exception {
        InodeAllocator allocator = new InodeAllocator(new MemoryIo());
        long first = allocator.allocate("foo");
        long second = allocator.allocate("foo");
        assertTrue(second > first);
    }
}