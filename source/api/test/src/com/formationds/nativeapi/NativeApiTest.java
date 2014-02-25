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
        Thread.sleep(1000);
        fds.getBucketsStats().get().forEach(b -> System.out.println(b));
        Thread.sleep(1000);
    }

    public void testPut() throws Exception {
        String bucketName = "slimebucket";
        //System.out.println("CreateBucket returned " + fds.createBucket(bucketName).get());
        //Thread.sleep(1000);
        byte[] bytes = {1, 2, 3, 4, 5};
        //System.out.println("Put bytes returned " + fds.put(bucketName, "thebytes", bytes).get());
        NativeApi.put(bucketName, "thebytes", bytes, i -> System.out.println("Callback value: " + i));
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

