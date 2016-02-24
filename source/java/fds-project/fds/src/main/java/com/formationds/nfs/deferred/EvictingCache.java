package com.formationds.nfs.deferred;

import com.formationds.nfs.SortableKey;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.*;

public class EvictingCache<TKey extends SortableKey<TKey>, TValue> {
    public static interface Loader<TKey, TValue> {
        public CacheEntry<TValue> load(TKey key) throws IOException;
    }

    private final Cache<TKey, CacheEntry<TValue>> cache;
    private Loader<TKey, TValue> loader;
    private Evictor<TKey, TValue> evictor;
    private String name;
    private final BlockingDeque<Exception> exceptions;
    private NavigableMap<TKey, CacheEntry<TValue>> traversableView;

    public EvictingCache(Loader<TKey, TValue> loader, Evictor<TKey, TValue> evictor, String name, int maxSize, int evictionInterval, TimeUnit evictionTimeUnit) {
        this.loader = loader;
        this.evictor = evictor;
        this.name = name;
        exceptions = new LinkedBlockingDeque<>();
        traversableView = new ConcurrentSkipListMap<>();

        cache = CacheBuilder.newBuilder()
                .maximumSize(maxSize)
                .expireAfterAccess(evictionInterval, evictionTimeUnit)
                .removalListener(notification -> {
                    RemovalCause cause = notification.getCause();
                    TKey key = (TKey) notification.getKey();
                    if (!(cause.equals(RemovalCause.EXPLICIT) || cause.equals(RemovalCause.REPLACED))) {
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

                    if (!cause.equals(RemovalCause.REPLACED)) {
                        traversableView.remove((TKey) notification.getKey());
                    }
                }).build();
    }

    public void start() {
        Thread thread = new Thread(() -> {
            while (true) {
                try {
                    Thread.sleep(1000);
                    cache.cleanUp();
                } catch (InterruptedException e) {
                    break;
                }
            }
        });
        thread.setName("NFS cache scavenger thread [" + name + "]");
        thread.start();
    }

    public CacheEntry<TValue> get(TKey key) throws IOException {
        bubbleExceptions();
        try {
            CacheEntry<TValue> cacheEntry = cache.get(key, () -> loader.load(key));
            traversableView.put(key, cacheEntry);
            return cacheEntry;
        } catch (ExecutionException e) {
            Throwable cause = e.getCause();
            if (cause instanceof IOException) {
                throw (IOException) cause;
            } else {
                throw new IOException(cause);
            }
        }
    }

    public void put(TKey key, CacheEntry<TValue> cacheEntry) {
        cache.put(key, cacheEntry);
        traversableView.put(key, cacheEntry);
    }

    public SortedMap<TKey, CacheEntry<TValue>> tailMap(TKey key) {
        return traversableView.tailMap(key);
    }

    public void flush(TKey key) throws IOException {
        bubbleExceptions();
        CacheEntry<TValue> cacheEntry = cache.getIfPresent(key);
        if (cacheEntry != null && cacheEntry.isDirty) {
            evictor.flush(key, cacheEntry);
            cacheEntry.isDirty = false;
        }
    }

    public void flush() throws IOException {
        bubbleExceptions();
        Set<TKey> keys = cache.asMap().keySet();
        for (TKey key : keys) {
            CacheEntry<TValue> entry = cache.asMap().get(key);
            if (entry.isDirty) {
                evictor.flush(key, entry);
                entry.isDirty = false;
            }
        }
    }

    public void remove(TKey key) throws IOException {
        bubbleExceptions();
        cache.asMap().remove(key);
        traversableView.remove(key);
    }

    public void dropKeysWithPrefix(TKey prefix) throws IOException {
        bubbleExceptions();
        Iterator<TKey> iterator = traversableView.tailMap(prefix).keySet().iterator();
        while (iterator.hasNext()) {
            TKey next = iterator.next();
            if (next.beginsWith(prefix)) {
                cache.asMap().remove(next);
                traversableView.remove(next);
            } else {
                break;
            }
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
}

