package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

abstract class ImageReader {
    private Counts counts;

    protected ImageReader() {
        counts = new Counts();
    }

    public Counts consumeCounts() {
        return counts.consume();
    }

    protected void increment(String volumeName) {
        counts.increment(volumeName);
    }

    protected abstract StoredImage read(StoredImage lastWritten) throws Exception;
}
