package com.formationds.nfs.deferred;

import java.io.IOException;

public interface Evictor<TKey, TValue> {
    public void flush(TKey key, TValue value) throws IOException;
}

