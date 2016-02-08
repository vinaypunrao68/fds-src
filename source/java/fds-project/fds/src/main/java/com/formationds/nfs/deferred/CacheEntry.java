package com.formationds.nfs.deferred;

public class CacheEntry<TValue> {
    public TValue value;
    public boolean isDirty;
    public boolean isDeleted;

    public CacheEntry(TValue value, boolean isDirty, boolean isDeleted) {
        this.value = value;
        this.isDirty = isDirty;
        this.isDeleted = isDeleted;
    }
}
