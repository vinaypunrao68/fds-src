package com.formationds.nfs;

import java.util.Map;

public class BlobMetadata {
    private String blobName;
    private Map<String, String> metadata;

    public BlobMetadata(String blobName, Map<String, String> metadata) {
        this.blobName = blobName;
        this.metadata = metadata;
    }

    public String getBlobName() {
        return blobName;
    }

    public Map<String, String> getMetadata() {
        return metadata;
    }
}
