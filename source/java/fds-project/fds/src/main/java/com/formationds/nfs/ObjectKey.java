package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

public class ObjectKey {
    String domain;
    String volume;
    String blobName;
    long objectOffset;

    public ObjectKey(String domain, String volume, String blobName, ObjectOffset objectOffset) {
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectOffset = objectOffset.getValue();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        ObjectKey objectKey = (ObjectKey) o;

        if (objectOffset != objectKey.objectOffset) return false;
        if (!blobName.equals(objectKey.blobName)) return false;
        if (!domain.equals(objectKey.domain)) return false;
        if (!volume.equals(objectKey.volume)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = domain.hashCode();
        result = 31 * result + volume.hashCode();
        result = 31 * result + blobName.hashCode();
        result = 31 * result + (int) (objectOffset ^ (objectOffset >>> 32));
        return result;
    }

    @Override
    public String toString() {
        return "ObjectKey{" +
                "domain='" + domain + '\'' +
                ", volume='" + volume + '\'' +
                ", blobName='" + blobName + '\'' +
                ", objectOffset=" + objectOffset +
                '}';
    }
}
