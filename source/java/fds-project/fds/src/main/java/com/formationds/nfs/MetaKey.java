package com.formationds.nfs;

class MetaKey {
    String domain;
    String volume;
    String blobName;

    MetaKey(String domain, String volume, String blobName) {
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MetaKey metaKey = (MetaKey) o;

        if (!blobName.equals(metaKey.blobName)) return false;
        if (!domain.equals(metaKey.domain)) return false;
        if (!volume.equals(metaKey.volume)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = domain.hashCode();
        result = 31 * result + volume.hashCode();
        result = 31 * result + blobName.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "MetaKey{" +
                "domain='" + domain + '\'' +
                ", volume='" + volume + '\'' +
                ", blobName='" + blobName + '\'' +
                '}';
    }
}
