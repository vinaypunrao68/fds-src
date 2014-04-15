package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.junit.Test;

import java.util.Iterator;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;

public class BlockWriterTest {
    @Test
    public void testWrite() throws Exception {
        BlockWriter outputStream = new BlockWriter(i -> null, () -> new Block(null, new byte[8]), 8);
        Iterator<Block> updated = outputStream.update(new byte[] {1, 2, 3, 4, 5}, 5, 4);
        assertArrayEquals(new byte[]{0, 0, 0, 0, 0, 1, 2, 3}, updated.next().getBytes());
        assertArrayEquals(new byte[]{4, 0, 0, 0, 0, 0, 0, 0}, updated.next().getBytes());
        assertFalse(updated.hasNext());
    }
}
