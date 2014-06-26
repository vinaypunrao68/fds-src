package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;

import java.util.Collections;
import java.util.List;

abstract class ImageWriter {
    private String[] volumeNames;
    private BucketStats bucketStats;

    public ImageWriter(String[] volumeNames, BucketStats bucketStats) {
        this.bucketStats = bucketStats;
        this.volumeNames = volumeNames;
    }

    public final BucketStats consumeCounts() {
        return bucketStats;
    }

    protected final void increment(String s) {
        bucketStats.increment(s);
    }

    public abstract StoredImage write(ImageResource resource);

    protected String randomVolume() {
        List<String> l = Lists.newArrayList(volumeNames);
        Collections.shuffle(l);
        return l.get(0);
    }
}
