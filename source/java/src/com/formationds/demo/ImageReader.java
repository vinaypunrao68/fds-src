package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

abstract class ImageReader {
    private BucketStats bucketStats;

    protected ImageReader(BucketStats bucketStats) {
        this.bucketStats = bucketStats;
    }

    public BucketStats counts() {
        return bucketStats;
    }

    protected void increment(String volumeName) {
        bucketStats.increment(volumeName);
    }

    protected abstract StoredImage read(StoredImage lastWritten) throws Exception;
}
