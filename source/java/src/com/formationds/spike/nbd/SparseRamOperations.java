package com.formationds.spike.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

public class SparseRamOperations implements NbdServerOperations {
    private long logicalSize;
    private Map<BlockMapKey, byte[]> blockMap;
    private static final int BLOCK_SIZE = 4096;

    class BlockMapKey {
        public String exportName;
        public long offset;

        public BlockMapKey(String exportName, long offset) {
            this.exportName = exportName;
            this.offset = offset;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            BlockMapKey blockMapKey = (BlockMapKey) o;

            if (offset != blockMapKey.offset) return false;
            if (!exportName.equals(blockMapKey.exportName)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = exportName.hashCode();
            result = 31 * result + (int) (offset ^ (offset >>> 32));
            return result;
        }
    }

    public SparseRamOperations(long logicalSize) {

        this.logicalSize = logicalSize;
        this.blockMap = new HashMap<>();
    }

    @Override
    public boolean exists(String exportName) {
        return true;
    }

    @Override
    public long size(String exportName) {
        return logicalSize;
    }

    private byte[] getBytes(String exportName, long offset) {
        BlockMapKey key = new BlockMapKey(exportName, offset);
        byte[] result = blockMap.getOrDefault(key, null);
        if(result != null)
            return result;

        byte[] buf = new byte[BLOCK_SIZE];
        blockMap.put(key, buf);
        return buf;
    }

    @Override
    public synchronized void read(String exportName, ByteBuf target, final long offset, int len) throws Exception {
        int objectSize = BLOCK_SIZE;

        int bytes_read = 0;
        while(bytes_read < len) {
            long cur = offset + bytes_read;
            long o_off = cur / BLOCK_SIZE;
            int i_off = (int)(cur % objectSize);
            int i_len = Math.min(len - bytes_read, objectSize - i_off);

            target.writeBytes(getBytes(exportName, o_off), i_off, i_len);
            bytes_read += i_len;
        }
    }

    @Override
    public synchronized void write(String exportName, ByteBuf source, long offset, int len) throws Exception {

        int bytes_written = 0;

        while(bytes_written < len) {
            long cur = offset + bytes_written;
            long o_off = cur / BLOCK_SIZE;
            int i_off = (int)(cur % BLOCK_SIZE);
            int i_len = Math.min(len - bytes_written, BLOCK_SIZE - i_off);

            source.getBytes(bytes_written, getBytes(exportName, o_off), i_off, i_len);
            bytes_written += i_len;

        }
    }
}
