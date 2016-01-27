package com.formationds.nfs;

import java.util.Arrays;

public class SimpleKey implements SortableKey<SimpleKey> {
    private byte[] bytes;

    public SimpleKey(String s) {
        bytes = s.getBytes();
    }

    @Override
    public byte[] bytes() {
        return bytes;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        SimpleKey simpleKey = (SimpleKey) o;

        if (!Arrays.equals(bytes, simpleKey.bytes)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(bytes);
    }
}
