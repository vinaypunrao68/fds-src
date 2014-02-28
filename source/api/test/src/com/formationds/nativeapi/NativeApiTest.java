package com.formationds.nativeapi;

import junit.framework.TestCase;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class NativeApiTest extends TestCase {
    private Fds fds;
    private ServerSetup serverSetup;

    public void testListBuckets() throws Exception {
        System.out.println("CreateBucket returned " + fds.createBucket("slimebucket").get());
        System.out.println("CreateBucket returned " + fds.createBucket("gutbucket").get());
        Thread.sleep(2000);
        fds.getBucketsStats().get().forEach(b -> System.out.println(b));
        Thread.sleep(1000);
    }

    public void testPut() throws Exception {
        String bucketName = "slimebucket";
        System.out.println("CreateBucket returned " + fds.createBucket(bucketName).get());
        Thread.sleep(4000);
        byte[] in = {1, 2, 3, 4};
        System.out.println("Put bytes returned " + fds.put(bucketName, "thebytes", in).get());
        byte[] out = {0, 0, 0, 0};
        System.out.println("Get bytes returned " + fds.get(bucketName, "thebytes", out).get());
    }

    @Override
    protected void setUp() {
        serverSetup = new ServerSetup();
        serverSetup.setUp();
        fds = new NativeApi();
    }

    public void tearDown() throws Exception {
        serverSetup.tearDown();
    }
}

