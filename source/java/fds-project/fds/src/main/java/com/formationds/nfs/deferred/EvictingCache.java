package com.formationds.nfs.deferred;

import com.formationds.util.IoFunction;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;

public class EvictingCache<TKey, TValue> {
    private final Cache<TKey, CacheEntry<TValue>> cache;
    private Cache<TKey, Object> locks;
    private Evictor<TKey, CacheEntry<TValue>> evictor;
    private String name;
    private final BlockingDeque<Exception> exceptions;

    public EvictingCache(Evictor<TKey, CacheEntry<TValue>> evictor, String name, int maxSize, int evictionInterval, TimeUnit evictionTimeUnit) {
        this.evictor = evictor;
        this.name = name;
        exceptions = new LinkedBlockingDeque<>();

        cache = CacheBuilder.newBuilder()
                .maximumSize(maxSize)
                .expireAfterAccess(evictionInterval, evictionTimeUnit)
                .removalListener(notification -> {
                    RemovalCause cause = notification.getCause();
                    if (!(cause.equals(RemovalCause.EXPLICIT) || cause.equals(RemovalCause.REPLACED))) {
                        TKey key = (TKey) notification.getKey();
                        CacheEntry<TValue> entry = (CacheEntry<TValue>) notification.getValue();
                        if (entry.isDirty) {
                            try {
                                evictor.flush(key, entry);
                                entry.isDirty = false;
                            } catch (Exception e) {
                                exceptions.add(e);
                            }
                        }
                    }
                }).build();

        locks = CacheBuilder.newBuilder()
                .maximumSize(maxSize)
                .expireAfterAccess(evictionInterval, evictionTimeUnit)
                .build();
    }

    public void start() {
        Thread thread = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000);
                    cache.cleanUp();
                    locks.cleanUp();
                } catch (InterruptedException e) {
                    break;
                }
            }
        });
        thread.setName("NFS cache scavenger thread [" + name + "]");
        thread.start();
    }

    public <T> T lock(TKey key, IoFunction<Map<TKey, CacheEntry<TValue>>, T> f) throws IOException {
        bubbleExceptions();
        Object lockObj = null;
        try {
            lockObj = locks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new IOException(e);
        }
        synchronized (lockObj) {
            return f.apply(this.cache.asMap());
        }
    }

    private void bubbleExceptions() throws IOException {
        List<Exception> exs = new ArrayList<>();
        exceptions.drainTo(exs);
        if (exs.size() != 0) {
            Exception exception = exs.get(0);
            if (exception instanceof IOException) {
                throw (IOException) exception;
            } else {
                throw new IOException(exception);
            }
        }
    }

    public void flush() throws IOException {
        Set<TKey> keys = cache.asMap().keySet();
        for (TKey key : keys) {
            lock(key, c -> {
                CacheEntry<TValue> entry = c.get(key);
                if (entry.isDirty) {
                    evictor.flush(key, entry);
                    entry.isDirty = false;
                }
                return null;
            });
        }
    }
}
