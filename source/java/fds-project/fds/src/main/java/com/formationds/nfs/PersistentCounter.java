package com.formationds.nfs;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.function.Function;

public class PersistentCounter {
    private TransactionalIo io;
    private final Cache<Key, Long> cache;
    private final Cache<Key, Object> locks;
    private final String counterName;
    private final long startValue;
    private boolean deferrable;

    public PersistentCounter(TransactionalIo io, String counterName, long startValue, boolean deferrable) {
        this.io = io;
        cache = CacheBuilder.newBuilder()
                .maximumSize(Long.MAX_VALUE)
                .concurrencyLevel(512)
                .build();
        locks = CacheBuilder.newBuilder()
                .maximumSize(Long.MAX_VALUE)
                .concurrencyLevel(512)
                .build();
        this.counterName = counterName;
        this.startValue = startValue;
        this.deferrable = deferrable;
    }

    public long increment(String volume, long step) throws IOException {
        return mutate(volume, old -> old + step);
    }

    public long decrement(String volume, long step) throws IOException {
        return mutate(volume, old -> old - step);
    }

    public long increment(String volume) throws IOException {
        return mutate(volume, old -> old + 1);
    }

    public long currentValue(String volume) throws IOException {
        synchronized (lockObject(volume)) {
            return pollCurrentValue(volume);
        }
    }

    private long mutate(String volume, Function<Long, Long> mutator) {
        synchronized (lockObject(volume)) {
            try {
                Key key = new Key(volume, counterName);
                Long oldValue = pollCurrentValue(volume);
                Map<String, String> map = new HashMap<>();
                Long newValue = mutator.apply(oldValue);
                cache.put(key, newValue);
                map.put(counterName, Long.toString(newValue));
                io.mutateMetadata(BlockyVfs.DOMAIN, volume, counterName, map, deferrable);
                return newValue;
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private Long pollCurrentValue(String volume) throws IOException {
        Key key = new Key(volume, counterName);
        Long oldValue = cache.getIfPresent(key);
        if (oldValue == null) {
            oldValue = io.mapMetadata(BlockyVfs.DOMAIN, volume, counterName,
                    (blobName, metadata) -> {
                        if (!metadata.isPresent()) {
                            return startValue;
                        }

                        String currentValue = metadata.get().getOrDefault(counterName, Long.toString(startValue));
                        return Long.parseLong(currentValue);
                    });
        }
        return oldValue;
    }

    private Object lockObject(String volume) {
        Key key = new Key(volume, counterName);
        try {
            return locks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private static class Key {
        private String volume;
        private String counterName;

        Key(String volume, String counterName) {
            this.volume = volume;
            this.counterName = counterName;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Key key = (Key) o;

            if (!counterName.equals(key.counterName)) return false;
            if (!volume.equals(key.volume)) return false;

            return true;
        }

        @Override
        public int hashCode() {
            int result = volume.hashCode();
            result = 31 * result + counterName.hashCode();
            return result;
        }
    }
}
