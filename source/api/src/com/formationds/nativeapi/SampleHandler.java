package com.formationds.nativeapi;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class SampleHandler implements BucketStatsHandler {
    public SampleHandler() {
    }

    @Override
    public void handle() {
        System.out.println("This is java code speaking");
    }
}
