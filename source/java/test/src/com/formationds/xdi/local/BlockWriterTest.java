package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class BlockWriterTest {
    @Test
    public void testWrite() throws Exception {
        int blockSize = 8;
        BlockWriter outputStream = new BlockWriter(i -> new Block(0l, i, new byte[blockSize]), blockSize);
        List<Block> blocks = Lists.newArrayList(outputStream.update(new byte[]{1, 2, 3, 4, 5}, 4, 5));
        assertEquals(2, blocks.size());
        assertArrayEquals(new byte[]{0, 0, 0, 0, 0, 1, 2, 3}, blocks.get(0).getBytes());
        assertArrayEquals(new byte[]{4, 0, 0, 0, 0, 0, 0, 0}, blocks.get(1).getBytes());
    }

    @Test
    public void testWriteNthBlock() throws Exception {
        int blockSize = 8;
        BlockWriter outputStream = new BlockWriter(i -> new Block(0l, i, new byte[blockSize]), blockSize);
        List<Block> blocks = Lists.newArrayList(outputStream.update(new byte[]{1, 2, 3, 4, 5}, 4, 8));
        assertEquals(1, blocks.size());
        assertArrayEquals(new byte[]{1, 2, 3, 4, 0, 0, 0, 0}, blocks.get(0).getBytes());
    }
}
