package com.formationds.nfs;

import com.formationds.util.IoConsumer;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

public class PersistentCounter implements IoConsumer<MetaKey> {
    private IoOps io;
    private String domain;
    private final String counterName;
    private final long startValue;
    private StripedLock lock;

    public PersistentCounter(IoOps io, String domain, String counterName, long startValue) {
        this.io = io;
        this.domain = domain;
        this.counterName = counterName;
        this.startValue = startValue;
        lock = new StripedLock();
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
        Map<String, String> metadata = io.readMetadata(domain, volume, counterName).orElse(new HashMap<String, String>());
        return Long.parseLong(metadata.getOrDefault(counterName, Long.toString(startValue)));
    }

    private long mutate(String volume, Function<Long, Long> mutator) throws IOException {
        return lock.lock(new MetaKey(domain, volume, counterName), () -> {
            Map<String, String> metadata = io.readMetadata(domain, volume, counterName).orElse(new HashMap<String, String>());
            long currentValue = Long.parseLong(metadata.getOrDefault(counterName, Long.toString(startValue)));
            long mutated = mutator.apply(currentValue);
            metadata.put(counterName, Long.toString(mutated));
            io.writeMetadata(domain, volume, counterName, metadata);
            return mutated;
        });
    }

    @Override
    public void accept(MetaKey tee) throws IOException {
        if (tee.domain.equals(domain) && tee.blobName.equals(counterName)) {
            // That's us - Do nothing
        } else {
            io.commitMetadata(domain, tee.volume, counterName);
        }
    }
}
