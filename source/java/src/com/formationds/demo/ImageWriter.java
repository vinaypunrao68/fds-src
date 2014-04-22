package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

interface ImageWriter {
    public Counts consumeCounts();
    public void write(ImageResource resource);
}
