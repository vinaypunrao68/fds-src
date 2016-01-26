package com.formationds.nfs;

public class BlobMetadata {
    private String blobName;
    private FdsMetadata metadata;

    public BlobMetadata(String blobName, FdsMetadata metadata) {
        this.blobName = blobName;
        this.metadata = metadata;
    }

    public String getBlobName() {
        return blobName;
    }

    public FdsMetadata getMetadata() {
        return metadata;
    }
}
