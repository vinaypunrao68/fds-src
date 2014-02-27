package com.formationds.nativeapi;

import java.util.Collection;
import java.util.concurrent.Future;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public interface Fds {
    public Future<Collection<BucketStatsContent>> getBucketsStats();
    public Future<Integer> createBucket(String bucketName);
    public Future<Integer> put(String bucketName, String objectName, byte[] bytes);
    public Future<Integer> get(String bucketName, String objectName, byte[] bytes);
}
