package com.formationds.spike.iscsi;

import org.jscsi.target.storage.IStorageModule;

import java.io.IOException;

public class MemoryStorageModule implements IStorageModule {

    private byte[] data;
    private int blockCount;

    public MemoryStorageModule(int blockCount) {
        this.blockCount = blockCount;
        data = new byte[VIRTUAL_BLOCK_SIZE * blockCount];
    }

    @Override
    public int checkBounds(long logicalBlockAddress, int transferLengthInBlocks) {
        if(blockCount <= logicalBlockAddress)
            return 1;
        if(blockCount <= logicalBlockAddress + transferLengthInBlocks)
            return 2;

        return 0;
    }

    @Override
    public long getSizeInBlocks() {
        return blockCount - 1; // i think the consumer of this method might be stupid about block boundaries
    }

    @Override
    public void read(byte[] bytes, long storageIndex) throws IOException {
        System.arraycopy(data, (int)storageIndex, bytes, 0, bytes.length);
    }

    @Override
    public void write(byte[] bytes, long storageIndex) throws IOException {
        System.arraycopy(bytes, 0, data, (int)storageIndex, bytes.length);
    }

    @Override
    public void close() throws IOException {

    }
}


