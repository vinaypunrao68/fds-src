package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.ByteArrayOutputStream;
import java.util.function.Function;

public class BlockReader {
    public byte[] read(Function<Integer, byte[]> blocks, int blocksize, long offset, int length) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        int start = (int) (offset % blocksize);
        int blockSpan = (int) Math.ceil((start + length) / (double) blocksize);
        int startingBlock = (int) Math.floor(offset / (double) blocksize);

        for (int i = 0; i < blockSpan; i++) {
            byte[] block = blocks.apply(i + startingBlock);
            int written = Math.min(blocksize - start, length);
            out.write(block, start, written);
            length -= written;
            start = 0;
        }
        return out.toByteArray();
    }
}