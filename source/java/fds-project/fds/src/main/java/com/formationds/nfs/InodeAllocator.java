package com.formationds.nfs;

import java.io.IOException;

public class InodeAllocator {
    private Io io;
    private static final String NUMBER_WELL = "number-well";
    public static final long START_VALUE = 256;

    public InodeAllocator(Io io) {
        this.io = io;
    }

    public long allocate(String volume) throws IOException {
        long[] nextId = new long[1];
        io.mutateMetadata(BlockyVfs.DOMAIN, volume, NUMBER_WELL, (meta) -> {
            long current = START_VALUE;
            if (meta.containsKey(NUMBER_WELL)) {
                current = Long.parseLong(meta.get(NUMBER_WELL));
            }
            nextId[0] = current + 1;
            meta.put(NUMBER_WELL, Long.toString(nextId[0]));
        });
        return nextId[0];
    }
}
