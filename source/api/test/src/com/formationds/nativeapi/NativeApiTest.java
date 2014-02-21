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
        BucketStatsHandler handler = new BucketStatsHandler() {
            @Override
            public void handle() {
                System.out.println("It worked");
            }
        };

        NativeApi.getBucketsStats(handler);
        Thread.sleep(4000);
    }
}
