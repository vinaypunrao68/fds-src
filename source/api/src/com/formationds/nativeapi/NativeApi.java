package com.formationds.nativeapi;

import java.io.File;
import java.util.Collection;
import java.util.function.Consumer;

/**
 * Created by fabrice on 2/19/14.
 */
public class NativeApi {
    static {
        String absolutePath = new File(".").getAbsolutePath();
        System.load(new File(absolutePath, "fds_java_bindings").getAbsolutePath());
    }

    public static native void init();
    public static native void getBucketsStats(Consumer<Collection<BucketStatsContent>> consumer);
    public static native void createBucket(String bucketName, Consumer<Integer> consumer);
}
