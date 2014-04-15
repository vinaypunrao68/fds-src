package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.ByteArrayInputStream;
import java.util.Iterator;
import java.util.function.Function;
import java.util.function.Supplier;

public class BlockWriter {
    private Function<Integer, Block> blockFinder;
    private int blockSize;
    private Supplier<Block> blockSupplier;

    public BlockWriter(Function<Integer, Block> blockFinder, Supplier<Block> newBlock, int blockSize) {
        this.blockFinder = blockFinder;
        this.blockSize = blockSize;
        this.blockSupplier = newBlock;
    }

    public Iterator<Block> update(byte[] bytes, final long offset, int length) {
        ByteArrayInputStream in = new ByteArrayInputStream(bytes, 0, length);
        byte[] buf = new byte[blockSize];

        return new Iterator<Block>() {
            long o = offset;
            byte[] b = bytes;
            int l = length;

            private int totalBlocks = (int) ((o + length) / blockSize);
            private int currentBlock = 0;

            @Override
            public boolean hasNext() {
                return currentBlock < totalBlocks;
            }

            @Override
            public Block next() {
                Block b = blockFinder.apply(currentBlock);
                if (b == null) {
                    b = blockSupplier.get();
                }

                if (o < blockSize) {
                    int read = in.read(buf, 0, (int) (blockSize - o));
                    if (read != -1) {
                        byte[] dest = b.getBytes();
                        System.arraycopy(buf, 0, dest, (int) o, (int) read);
                        b.setBytes(dest);
                    }
                    o = 0;
                } else {
                    o -= blockSize;
                }

                currentBlock++;
                return b;
            }
        };
    }
}
