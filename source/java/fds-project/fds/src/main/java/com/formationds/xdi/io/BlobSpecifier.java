package com.formationds.xdi.io;

import com.formationds.xdi.s3.S3Namespace;

public class BlobSpecifier {
    private String domainName;
    private String volumeName;
    private String blobName;

    public BlobSpecifier(String domainName, String volumeName, String blobName) {
        this.domainName = domainName;
        this.volumeName = volumeName;
        this.blobName = blobName;
    }

    public String getDomainName() {
        return domainName;
    }

    public String getVolumeName() {
        return volumeName;
    }

    public String getBlobName() {
        return blobName;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        BlobSpecifier that = (BlobSpecifier) o;

        if (!blobName.equals(that.blobName)) return false;
        if (!domainName.equals(that.domainName)) return false;
        if (!volumeName.equals(that.volumeName)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = domainName.hashCode();
        result = 31 * result + volumeName.hashCode();
        result = 31 * result + blobName.hashCode();
        return result;
    }
}
