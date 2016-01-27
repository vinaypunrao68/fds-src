package com.formationds.nfs;

import com.google.common.primitives.UnsignedBytes;

public interface SortableKey<T> extends Comparable<SortableKey<T>> {
    public byte[] bytes();

    public default boolean beginsWith(SortableKey<T> prefix) {
        byte[] myBytes = bytes();
        byte[] theirBytes = prefix.bytes();

        if (myBytes.length < theirBytes.length) {
            return false;
        }

        for (int i = 0; i < theirBytes.length; i++) {
            if (myBytes[i] != theirBytes[i]) {
                return false;
            }
        }

        return true;
    }

    @Override
    public default int compareTo(SortableKey<T> o) {
        return UnsignedBytes.lexicographicalComparator().compare(this.bytes(), o.bytes());
    }
}

