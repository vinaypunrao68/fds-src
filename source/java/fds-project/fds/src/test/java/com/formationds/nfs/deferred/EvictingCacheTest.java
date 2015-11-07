package com.formationds.nfs.deferred;

import org.junit.Test;

import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class EvictingCacheTest {
    @Test
    public void testFlush() throws Exception {
        EvictingCache<Integer, String> cache = new EvictingCache<>(
                (key, entry) -> entry.isDirty = false,
                "test", 100, 1, TimeUnit.HOURS
        );

        cache.lock(1, c -> c.put(1, new CacheEntry<>("foo", true)));
        assertTrue(cache.lock(1, c -> c.get(1).isDirty));
        cache.flush();
        assertFalse(cache.lock(1, c -> c.get(1).isDirty));
    }
}