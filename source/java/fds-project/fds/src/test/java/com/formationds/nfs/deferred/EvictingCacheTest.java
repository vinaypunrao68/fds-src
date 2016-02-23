package com.formationds.nfs.deferred;

import com.formationds.nfs.SimpleKey;
import org.junit.Test;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;

public class EvictingCacheTest {
    @Test
    public void test() throws Exception {
        AtomicInteger loads = new AtomicInteger(0);
        AtomicInteger evictions = new AtomicInteger(0);

        EvictingCache.Loader<SimpleKey, Integer> loader =
                simpleKey -> new CacheEntry<>(loads.incrementAndGet(), true, false);

        Evictor<SimpleKey, Integer> evictor =
                (simpleKey, cacheEntry) -> evictions.incrementAndGet();

        EvictingCache<SimpleKey, Integer> cache = new EvictingCache<>(
                loader, evictor, "foo", 42, 10, TimeUnit.SECONDS
        );

        cache.start();

        SimpleKey key = new SimpleKey("foo");
        CacheEntry<Integer> value = cache.get(key);
        assertEquals(1, (int) value.value);

        cache.put(key, new CacheEntry<>(42, true, false));
        assertEquals(42, (int) cache.get(key).value);
        assertEquals(1, loads.get());

        cache.flush(key);
        assertEquals(1, evictions.get());

        cache.put(key, new CacheEntry<>(42, true, true));
        cache.flush();
        assertEquals(2, evictions.get());
    }
}