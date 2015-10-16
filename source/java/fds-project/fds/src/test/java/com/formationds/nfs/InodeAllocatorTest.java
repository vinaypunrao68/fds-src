package com.formationds.nfs;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class InodeAllocatorTest {
    @Test
    public void testAllocate() throws Exception {
        TransactionalIo transactions = new TransactionalIo(new MemoryIoOps());
        InodeAllocator allocator = new InodeAllocator(transactions);
        long first = allocator.allocate("foo");
        long second = allocator.allocate("foo");
        assertTrue(second > first);

        // test restart
        allocator = new InodeAllocator(transactions);
        long third = allocator.allocate("foo");
        assertTrue(third > second);
    }
}