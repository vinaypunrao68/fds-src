package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class InodeAllocatorTest {
    @Test
    public void testAllocate() throws Exception {
        InodeAllocator allocator = new InodeAllocator(new MemoryIo(), new StubExportResolver("foo", 8));
        long first = allocator.allocate("foo");
        long second = allocator.allocate("foo");
        assertTrue(second > first);
    }
}