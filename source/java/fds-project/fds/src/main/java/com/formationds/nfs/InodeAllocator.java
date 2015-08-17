package com.formationds.nfs;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class InodeAllocator {
    private Io io;
    private ExportResolver exportResolver;
    private static final String NUMBER_WELL = "number-well";

    public InodeAllocator(Io io, ExportResolver exportResolver) {
        this.io = io;
        this.exportResolver = exportResolver;
    }

    public long allocate(String volume) throws IOException {
        long volumeId = exportResolver.exportId(volume);
        long[] nextId = new long[1];
        io.mutateMetadata(BlockyVfs.DOMAIN, volume, NUMBER_WELL, (om) -> {
            Map<String, String> metadata = new HashMap<>();
            long current = volumeId;
            if (om.isPresent()) {
                current = Long.parseLong(om.get().get(NUMBER_WELL));
            }
            nextId[0] = current + 1;
            metadata.put(NUMBER_WELL, Long.toString(nextId[0]));
            return metadata;
        });
        return nextId[0];
    }
}
