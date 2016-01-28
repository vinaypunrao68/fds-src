package com.formationds.nfs;

import com.google.common.primitives.UnsignedBytes;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public abstract class SortableKey<T> implements Comparable<SortableKey<T>> {
    public final byte[] bytes;

    public SortableKey(String... elements) {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream daos = new DataOutputStream(baos);
        try {
            for (String element : elements) {
                daos.writeBytes("/");
                daos.writeBytes(element);
            }
            daos.flush();
            bytes = baos.toByteArray();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public boolean beginsWith(SortableKey<T> prefix) {
        byte[] myBytes = bytes;
        byte[] theirBytes = prefix.bytes;

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
    public int compareTo(SortableKey<T> o) {
        return UnsignedBytes.lexicographicalComparator().compare(this.bytes, o.bytes);
    }
}


