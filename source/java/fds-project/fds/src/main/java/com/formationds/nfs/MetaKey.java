package com.formationds.nfs;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;

public class MetaKey implements SortableKey<MetaKey> {
    final String domain;
    final String volume;
    final String blobName;
    final byte[] bytes;

    public MetaKey(String domain, String volume, String blobName) {
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream daos = new DataOutputStream(baos);
        try {
            daos.write(domain.getBytes());
            daos.write(volume.getBytes());
            daos.write(blobName.getBytes());
            daos.flush();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        this.bytes = baos.toByteArray();
    }

    @Override
    public byte[] bytes() {
        return bytes;
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
