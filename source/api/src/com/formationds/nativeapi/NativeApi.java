package com.formationds.nativeapi;

import java.io.File;
import java.util.Collection;
import java.util.concurrent.Future;
import java.util.function.Consumer;

/**
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class NativeApi implements Fds {
    static {
        String absolutePath = new File(".").getAbsolutePath();
        System.load(new File(absolutePath, "fds_java_bindings").getAbsolutePath());
    }

    static native void init();
    static native void getBucketsStats(Consumer<Collection<BucketStatsContent>> result);
    static native void createBucket(String bucketName, Consumer<Integer> result);
    public static native void put(String bucketName, String objectName, byte[] bytes, Consumer<Integer> result);

    static {
        init();
    }

    @Override
    public Future<Collection<BucketStatsContent>> getBucketsStats() {
        Acceptor<Collection<BucketStatsContent>> acceptor = new Acceptor<>();
        getBucketsStats(acceptor);
        return acceptor;
    }

    @Override
    public Future<Integer> createBucket(String bucketName) {
        Acceptor<Integer> acceptor = new Acceptor<>();
        createBucket(bucketName, acceptor);
        return acceptor;
    }

    @Override
    public Future<Integer> put(String bucketName, String objectName, byte[] bytes) {
        Acceptor<Integer> acceptor = new Acceptor<>();
        put(bucketName, objectName, bytes, acceptor);
        return acceptor;
    }
}
