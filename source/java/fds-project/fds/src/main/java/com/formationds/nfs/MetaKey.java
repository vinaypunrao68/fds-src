package com.formationds.nfs;

import java.util.Arrays;

public class MetaKey extends SortableKey<MetaKey> {
    String domain;
    String volume;
    String blobName;

    public MetaKey(String domain, String volume, String blobName) {
        super(domain, volume, blobName);
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
    }

    public MetaKey(String domain, String volume) {
        super(domain, volume);
        this.blobName = "";
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MetaKey metaKey = (MetaKey) o;

        if (!Arrays.equals(bytes, metaKey.bytes)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(bytes);
    }


    @Override
    public String toString() {
        return String.format("[MetaKey domain=%s, volume=%s, blobName=%s]", domain, volume, blobName);

    }
}
