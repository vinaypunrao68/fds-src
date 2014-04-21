package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.junit.Before;
import org.junit.Test;

import java.util.List;
import java.util.function.Function;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

public class BlockReaderTest {

    private List<Block> blocks;
    public static final int BLOCK_SIZE = 4;
    private Function<Integer, byte[]> blockFactory;

    @Test
    public void testRead() {
        BlockReader blockReader = new BlockReader();
        byte[] result = blockReader.read(blockFactory, BLOCK_SIZE, 3, 4);
        assertArrayEquals(new byte[]{3, 4, 5, 6}, result);

        result = blockReader.read(blockFactory, 4, 12, 4);
        assertEquals(4, result.length);
    }

    @Before
    public void setUp() throws Exception {
        blocks = Lists.newArrayList(
                new Block(0, 0, new byte[]{0, 1, 2, 3}),
                new Block(0, 1, new byte[]{4, 5, 6, 7}),
                new Block(0, 2, new byte[]{8, 9, 10, 11}),
                new Block(0, 3, new byte[]{12, 13, 14, 15})
        );
        blockFactory = i -> i >= blocks.size() ? new byte[BLOCK_SIZE] : blocks.get(i).getBytes();
    }
}
