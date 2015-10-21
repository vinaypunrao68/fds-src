package com.formationds.nfs;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

public class InodeAllocator {
    private TransactionalIo io;
    private static final String NUMBER_WELL = "number-well";
    public static final long START_VALUE = 256;
    private final ConcurrentHashMap<String, Long> cache;

    public InodeAllocator(TransactionalIo io) {
        this.io = io;
        cache = new ConcurrentHashMap<>(256, .5f, 512);
    }

    public long allocate(String volume) throws IOException {
        return cache.compute(volume, (k, v) -> {
            long newValue = 0;
            try {
                if (v == null) {
                    v = io.mapMetadata(BlockyVfs.DOMAIN, volume, NUMBER_WELL, new MetadataMapper<Long>() {
                        @Override
                        public Long map(Optional<Map<String, String>> metadata) throws IOException {
                            if (!metadata.isPresent()) {
                                return START_VALUE;
                            }

                            String currentValue = metadata.get().getOrDefault(NUMBER_WELL, Long.toString(START_VALUE));
                            return Long.parseLong(currentValue);
                        }
                    });
                }

                Map<String, String> map = new HashMap<>();
                newValue = v + 1;
                map.put(NUMBER_WELL, Long.toString(newValue));
                io.mutateMetadata(BlockyVfs.DOMAIN, volume, NUMBER_WELL, map, false);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
            return newValue;
        });
    }
}
