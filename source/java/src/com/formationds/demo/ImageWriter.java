package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;

import java.util.Collections;
import java.util.List;

abstract class ImageWriter {
    private String[] volumeNames;
    private Counts counts;

    public ImageWriter(String[] volumeNames) {
        counts = new Counts();
        this.volumeNames = volumeNames;
    }

    public final Counts consumeCounts() {
        return counts.consume();
    }

    protected final void increment(String s) {
        counts.increment(s);
    }

    public String[] getVolumeNames() {
        return volumeNames;
    }

    public abstract StoredImage write(ImageResource resource);

    protected String randomVolume() {
        List<String> l = Lists.newArrayList(volumeNames);
        Collections.shuffle(l);
        return l.get(0);
    }
}
