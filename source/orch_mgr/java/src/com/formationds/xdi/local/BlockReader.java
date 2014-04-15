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
        int blockSpan = (length / blocksize) + 1;
        for (int i = (int) (offset / blocksize); i < blockSpan; i++) {
            byte[] block = blocks.apply(i);
            int written = Math.min(blocksize - start, length);
            out.write(block, start, written);
            length -= written;
            start = 0;
        }
        return out.toByteArray();
}

}