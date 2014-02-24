package com.formationds.nativeapi;

import junit.framework.TestCase;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class NativeApiTest extends TestCase {
    private Fds fds;

    public void testListBuckets() throws Exception {
        System.out.println("CreateBucket returned " + fds.createBucket("slimebucket").get());
        Thread.sleep(1000);
        fds.getBucketsStats().get().forEach(b -> System.out.println(b));
        Thread.sleep(1000);
    }

    public void testPut() throws Exception {
        String bucketName = "slimebucket";
        System.out.println("CreateBucket returned " + fds.createBucket(bucketName).get());
        Thread.sleep(1000);
        byte[] bytes = {1, 2, 3, 4, 5};
        System.out.println("Put bytes returned " + fds.put(bucketName, "thebytes", bytes).get());
        Thread.sleep(1000);
    }

    @Override
    protected void setUp() throws Exception {
        fds = new NativeApi();
    }
}
