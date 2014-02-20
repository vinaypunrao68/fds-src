package com.formationds.nativeapi;

import junit.framework.TestCase;

import java.util.Collection;

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
            public void handle(String timestamp, Collection<BucketStatsContent> contents) {
                System.out.println("Got " + contents + " buckets");
            }
        };

        NativeApi.getBucketsStats(handler);
    }
}
