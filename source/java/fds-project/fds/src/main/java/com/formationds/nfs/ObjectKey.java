package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;

public class ObjectKey implements SortableKey<ObjectKey> {
    final String domain;
    final String volume;
    final String blobName;
    final long objectOffset;

    private byte[] bytes;

    public ObjectKey(String domain, String volume, String blobName, ObjectOffset objectOffset) {
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectOffset = objectOffset.getValue();

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream daos = new DataOutputStream(baos);
        try {
            daos.write(domain.getBytes());
            daos.write(volume.getBytes());
            daos.write(blobName.getBytes());
            if (objectOffset.getValue() != 0) {
                daos.writeLong(objectOffset.getValue());
            }
            daos.flush();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        this.bytes = baos.toByteArray();
    }

    public byte[] bytes() {
        return bytes;
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
