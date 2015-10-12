package com.formationds.nfs.deferred;

public class CacheEntry<TValue> {
    public TValue value;
    public boolean isDirty;

    public CacheEntry(TValue value, boolean isDirty) {
        this.value = value;
        this.isDirty = isDirty;
    }
}
