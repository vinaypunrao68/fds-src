package com.formationds.nfs.deferred;

import com.formationds.nfs.SimpleKey;
import org.junit.Test;

import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.*;

public class EvictingCacheTest {
    @Test
    public void testConcurrentBehaviorSanity() throws Exception {
        AtomicInteger evictionCount = new AtomicInteger(0);
        EvictingCache<SimpleKey, String> cache = new EvictingCache<>((o, ce) -> {
            evictionCount.incrementAndGet();

        }, "foo", 100, 1, TimeUnit.HOURS);
        ExecutorService pool = Executors.newFixedThreadPool(8);
        SimpleKey key = new SimpleKey("hello");

        cache.lock(key, m -> {
            m.put(key, new CacheEntry<>("world", true, false));
            return null;
        });

        AtomicInteger invocationCount = new AtomicInteger(0);

        for (int i = 0; i < 1024 * 8; i++) {
            pool.submit(() -> {
                assertNotNull(cache.unlockedRead(key, c -> c.get(key)));
                return cache.lock(key, m -> {
                    m.put(new SimpleKey(UUID.randomUUID().toString()), new CacheEntry<>(UUID.randomUUID().toString(), true, false));
                    m.put(key, new CacheEntry<>(UUID.randomUUID().toString(), true, false));
                    invocationCount.incrementAndGet();
                    return null;
                });
            });
        }

        pool.shutdown();
        pool.awaitTermination(1, TimeUnit.SECONDS);
        assertEquals(8 * 1024, invocationCount.get());
        assertTrue(evictionCount.get() > 0);
    }

    @Test
    public void testFlush() throws Exception {
        EvictingCache<SimpleKey, String> cache = new EvictingCache<>(
                (key, entry) -> {},
                "test", 100, 1, TimeUnit.HOURS
        );

        SimpleKey key = new SimpleKey("foo");
        cache.lock(key, c -> c.put(key, new CacheEntry<>("foo", true, false)));
        assertTrue(cache.lock(key, c -> c.get(key).isDirty));
        cache.flush();
        assertFalse(cache.lock(key, c -> c.get(key).isDirty));
    }

    @Test
    public void testDropKeysWithPrefix() throws Exception {
        EvictingCache<SimpleKey, String> cache = new EvictingCache<>((o, ce) -> {
        }, "foo", 1000, 1, TimeUnit.HOURS);

        SimpleKey someKey = new SimpleKey("");
        cache.lock(someKey, c -> {
            c.put(new SimpleKey("aaa"), new CacheEntry<>("aaa", true, false));
            c.put(new SimpleKey("aab"), new CacheEntry<>("aab", true, false));
            c.put(new SimpleKey("abb"), new CacheEntry<>("abb", true, false));
            return null;
        });

        assertTrue(cache.lock(someKey, c -> c.get(new SimpleKey("aaa")) != null));
        assertTrue(cache.lock(someKey, c -> c.get(new SimpleKey("aab")) != null));
        cache.dropKeysWithPrefix(new SimpleKey("aa"));
        assertTrue(cache.lock(someKey, c -> c.get(new SimpleKey("aaa")) == null));
        assertTrue(cache.lock(someKey, c -> c.get(new SimpleKey("aab")) == null));
        assertTrue(cache.lock(someKey, c -> c.get(new SimpleKey("abb")) != null));
    }
}