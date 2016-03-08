package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.util.Arrays;

public class ObjectKey extends SortableKey<ObjectKey> {
    String domain;
    String volume;
    String blobName;
    long objectOffset;

    public ObjectKey(String domain, String volume, String blobName, ObjectOffset objectOffset) {
        super(domain, volume, blobName, Long.toString(objectOffset.getValue()));
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectOffset = objectOffset.getValue();
    }

    public ObjectKey(String domain, String volume) {
        super(domain, volume);
        this.domain = domain;
        this.volume = volume;
        this.blobName = "";
        this.objectOffset = 0;
    }

    public ObjectKey(String domain, String volume, String blobName) {
        super(domain, volume, blobName);
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectOffset = 0;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        ObjectKey objectKey = (ObjectKey) o;

        if (!Arrays.equals(bytes, objectKey.bytes)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        return Arrays.hashCode(bytes);
    }

    @Override
    public String toString() {
        return String.format("[ObjectKey domain=%s, volume=%s, blobName=%s, objectOffset=%s]", domain, volume, blobName, Long.toString(objectOffset));
    }
}
