package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.List;
import java.util.function.Function;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class BlockReaderTest {
    @Test
    public void testRead() {
        List<Block> blocks = Lists.newArrayList(
                new Block(null, new byte[]{0, 1, 2, 3}),
                new Block(null, new byte[]{4, 5, 6, 7}),
                new Block(null, new byte[]{8, 9, 10, 11}),
                new Block(null, new byte[]{12, 13, 14, 15})
        );

        int blockSize = 4;
        Function<Integer, byte[]> f = i -> i >= blocks.size() ? new byte[blockSize] : blocks.get(i).getBytes();

        BlockReader blockReader = new BlockReader();

        byte[] result = blockReader.read(f, blockSize, 3, 4);
        assertArrayEquals(new byte[]{3, 4, 5, 6}, result);

        result = blockReader.read(f, 4, 12, 4);
        assertEquals(4, result.length);
    }
}
