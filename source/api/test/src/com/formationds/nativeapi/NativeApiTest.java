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
        NativeApi.createBucket("请收藏我们的网址", i -> System.out.println("bucket creation returned " + i));
        Thread.sleep(5000);
        NativeApi.getBucketsStats(buckets -> buckets.forEach(b -> System.out.println(b)));
        Thread.sleep(5000);
    }
}
