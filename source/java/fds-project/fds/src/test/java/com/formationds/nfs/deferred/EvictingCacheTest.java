package com.formationds.nfs.deferred;

import com.formationds.nfs.SimpleKey;
import org.junit.Test;

import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class EvictingCacheTest {
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