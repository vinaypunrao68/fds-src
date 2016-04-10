package com.formationds.nfs.deferred;

import com.formationds.nfs.SimpleKey;
import org.junit.Before;
import org.junit.Test;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.*;

public class EvictingCacheTest {

    private EvictingCache<SimpleKey, Integer> cache;
    private AtomicInteger loads;
    private AtomicInteger evictions;

    @Test
    public void testFlushKeysWithPrefix() throws Exception {
        assertTrue(cache.get(new SimpleKey("aaa")).isDirty);
        assertTrue(cache.get(new SimpleKey("bbb")).isDirty);
        assertTrue(cache.get(new SimpleKey("bbc")).isDirty);
        assertTrue(cache.get(new SimpleKey("ccc")).isDirty);

        assertEquals(0, evictions.get());
        assertEquals(4, loads.get());

        cache.flush(new SimpleKey("bbb"));
        assertEquals(1, evictions.get());

        cache.flushKeysWithPrefix(new SimpleKey("b"));
        assertTrue(cache.get(new SimpleKey("aaa")).isDirty);
        assertFalse(cache.get(new SimpleKey("bbb")).isDirty);
        assertFalse(cache.get(new SimpleKey("bbc")).isDirty);
        assertTrue(cache.get(new SimpleKey("ccc")).isDirty);
        assertEquals(2, evictions.get());
    }

    @Test
    public void testBasic() throws Exception {
        SimpleKey key = new SimpleKey("foo");
        CacheEntry<Integer> value = cache.get(key);
        assertEquals(1, (int) value.value);

        cache.put(key, new CacheEntry<>(42, true, false));
        assertEquals(42, (int) cache.get(key).value);
        assertEquals(1, loads.get());

        cache.flush(key);
        assertEquals(1, evictions.get());

        cache.put(key, new CacheEntry<>(42, true, true));
        cache.flushAll();
        assertEquals(2, evictions.get());
    }

    @Before
    public void setUp() throws Exception {
        loads = new AtomicInteger(0);
        evictions = new AtomicInteger(0);

        EvictingCache.Loader<SimpleKey, Integer> loader =
                simpleKey -> new CacheEntry<>(loads.incrementAndGet(), true, false);

        Evictor<SimpleKey, Integer> evictor =
                (simpleKey, cacheEntry) -> evictions.incrementAndGet();

        cache = new EvictingCache<>(
                loader, evictor, "foo", 42, 10, TimeUnit.SECONDS
        );

        cache.start();
    }
}