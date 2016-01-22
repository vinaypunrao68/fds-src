package com.formationds.nfs;

import com.google.common.primitives.UnsignedBytes;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;

class MetaKey implements Comparable<MetaKey> {
    String domain;
    String volume;
    String blobName;
    private byte[] bytes;

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
    public int compareTo(MetaKey o) {
        return UnsignedBytes.lexicographicalComparator().compare(this.bytes, o.bytes);
    }

    public boolean beginsWith(MetaKey prefix) {
        if (bytes.length < prefix.bytes.length) {
            return false;
        }

        for (int i = 0; i < prefix.bytes.length; i++) {
            if (bytes[i] != prefix.bytes[i]) {
                return false;
            }
        }

        return true;
    }
}
