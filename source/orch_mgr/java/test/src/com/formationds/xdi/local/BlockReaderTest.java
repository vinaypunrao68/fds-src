package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.List;
import java.util.function.Function;

import static org.junit.Assert.assertArrayEquals;

public class BlockReaderTest {
    @Test
    public void testRead() {
        List<Block> blocks = Lists.newArrayList(
                new Block(null, new byte[]{0, 1, 2, 3}),
                new Block(null, new byte[]{4, 5, 6, 7})
        );

        int blockSize = 4;
        Function<Integer, byte[]> f = i -> i >= blocks.size() ? new byte[blockSize] : blocks.get(i).getBytes();
        byte[] result = new BlockReader().read(f, blockSize, 3, 4);
        assertArrayEquals(new byte[]{3, 4, 5, 6}, result);
    }
}
