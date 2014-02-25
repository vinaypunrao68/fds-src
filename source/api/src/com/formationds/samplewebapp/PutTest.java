package com.formationds.samplewebapp;

import baggage.hypertoolkit.RequestHandler;
import baggage.hypertoolkit.views.Resource;
import baggage.hypertoolkit.views.TextResource;
import com.formationds.nativeapi.Fds;

import javax.servlet.http.HttpServletRequest;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class PutTest implements RequestHandler {
    private Fds fds;

    public PutTest(Fds fds) {
        this.fds = fds;
    }

    @Override
    public Resource handle(HttpServletRequest request) throws Exception {
        String bucketName = "slimebucket";
        byte[] bytes = {0, 1, 1, 2, 3, 5, 8, 13};
        Integer createBucketResult = fds.createBucket(bucketName).get();
        Thread.sleep(1000);
        Integer putResult = fds.put(bucketName, "mybytes", bytes).get();
        return new TextResource("CreateBucket: " + createBucketResult + ", Put: " + putResult);
    }

    @Override
    public boolean log() {
        return false;
    }
}
