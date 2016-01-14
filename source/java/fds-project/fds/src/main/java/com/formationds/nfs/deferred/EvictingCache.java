package com.formationds.nfs.deferred;

import com.formationds.util.IoFunction;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.RemovalCause;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.*;

public class EvictingCache<TKey, TValue> {
    private final Cache<TKey, CacheEntry<TValue>> cache;
    private Cache<TKey, Object> locks;
    private TraversableView traversableView;
    private Evictor<TKey, CacheEntry<TValue>> evictor;
    private String name;
    private final BlockingDeque<Exception> exceptions;

    public EvictingCache(Evictor<TKey, CacheEntry<TValue>> evictor, String name, int maxSize, int evictionInterval, TimeUnit evictionTimeUnit) {
        this.evictor = evictor;
        this.name = name;
        exceptions = new LinkedBlockingDeque<>();
        traversableView = new TraversableView();

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

                    if (!cause.equals(RemovalCause.REPLACED)) {
                        traversableView.evict((TKey) notification.getKey());
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

    public <T> T lock(TKey key, IoFunction<SortedMap<TKey, CacheEntry<TValue>>, T> f) throws IOException {
        bubbleExceptions();
        Object lockObj = null;
        try {
            lockObj = locks.get(key, () -> new Object());
        } catch (ExecutionException e) {
            throw new IOException(e);
        }
        synchronized (lockObj) {
            return f.apply(this.traversableView);
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


    class TraversableView implements SortedMap<TKey, CacheEntry<TValue>> {
        private NavigableMap<TKey, CacheEntry<TValue>> inner = new ConcurrentSkipListMap<>();

        @Override
        public int size() {
            return cache.asMap().size();
        }

        @Override
        public boolean isEmpty() {
            return cache.asMap().isEmpty();
        }

        @Override
        public boolean containsKey(Object key) {
            return cache.asMap().containsKey(key);
        }

        @Override
        public boolean containsValue(Object value) {
            return cache.asMap().containsValue(value);
        }

        @Override
        public CacheEntry<TValue> get(Object key) {
            return cache.asMap().get(key);
        }

        @Override
        public CacheEntry<TValue> put(TKey key, CacheEntry<TValue> value) {
            cache.put(key, value);
            return inner.put(key, value);
        }

        @Override
        public CacheEntry<TValue> remove(Object key) {
            cache.asMap().remove(key);
            return inner.remove(key);
        }

        CacheEntry<TValue> evict(Object key) {
            return inner.remove(key);
        }

        @Override
        public void putAll(Map<? extends TKey, ? extends CacheEntry<TValue>> m) {
            cache.putAll(m);
            inner.putAll(m);
        }

        @Override
        public void clear() {
            cache.asMap().clear();
            inner.clear();
        }

        @Override
        public Comparator<? super TKey> comparator() {
            return inner.comparator();
        }

        @Override
        public SortedMap<TKey, CacheEntry<TValue>> subMap(TKey fromKey, TKey toKey) {
            return inner.subMap(fromKey, toKey);
        }

        @Override
        public SortedMap<TKey, CacheEntry<TValue>> headMap(TKey toKey) {
            return inner.headMap(toKey);
        }

        @Override
        public SortedMap<TKey, CacheEntry<TValue>> tailMap(TKey fromKey) {
            return inner.tailMap(fromKey);
        }

        @Override
        public TKey firstKey() {
            return inner.firstKey();
        }

        @Override
        public TKey lastKey() {
            return inner.lastKey();
        }

        @Override
        public Set<TKey> keySet() {
            return cache.asMap().keySet();
        }

        @Override
        public Collection<CacheEntry<TValue>> values() {
            return cache.asMap().values();
        }

        @Override
        public Set<Entry<TKey, CacheEntry<TValue>>> entrySet() {
            return cache.asMap().entrySet();
        }
    }
}

