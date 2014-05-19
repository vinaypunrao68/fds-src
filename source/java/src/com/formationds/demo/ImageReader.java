package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

interface ImageReader {
    public Counts consumeCounts();
    public StoredImage read(StoredImage lastWritten) throws Exception;
}
