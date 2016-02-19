package com.formationds.nfs;

import com.formationds.util.IoFunction;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.locks.ReentrantLock;

public class FdsMetadata {
    private final Map<String, String> map;
    private final ReentrantLock lock;

    public FdsMetadata() {
        this(new HashMap<>());
    }

    public FdsMetadata(Map<String, String> map) {
        this.map = new HashMap<>(map);
        lock = new ReentrantLock();
    }

    public <T> T lock(IoFunction<Accessor, T> f) throws IOException {
        lock.lock();
        try {
            return f.apply(new Accessor());
        } finally {
            lock.unlock();
        }
    }

    public class Accessor {
        public Map<String, String> mutableMap() {
            return map;
        }

        public FdsMetadata fdsMetadata() {
            return FdsMetadata.this;
        }
    }

    /**
     * Not thread-safe, use lock()
     */
    public int fieldCount() {
        return map.size();
    }
}
