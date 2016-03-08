package com.formationds.nfs.deferred;

import java.io.IOException;

public interface Evictor<TKey, TValue> {
    public void flush(TKey key, CacheEntry<TValue> cacheEntry) throws IOException;
}

