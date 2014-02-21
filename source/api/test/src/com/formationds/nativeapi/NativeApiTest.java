package com.formationds.nativeapi;

import junit.framework.TestCase;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class NativeApiTest extends TestCase {
    public void testInit() throws Exception {
        NativeApi.init();
    }

    public void testGetBucketStats() throws Exception {
        NativeApi.init();
        NativeApi.getBucketsStats(buckets -> System.out.println("Bucket count: " + buckets.size()));
        Thread.sleep(4000);
    }
}
