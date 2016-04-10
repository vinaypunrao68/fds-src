package com.formationds.nfs.deferred;

public class CacheEntry<TValue> {
    public TValue value;
    public boolean isDirty;
    public boolean isMissing;

    public CacheEntry(TValue value, boolean isDirty, boolean isMissing) {
        this.value = value;
        this.isDirty = isDirty;
        this.isMissing = isMissing;
    }
}
