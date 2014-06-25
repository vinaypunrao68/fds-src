package com.formationds.demo;

import java.util.concurrent.TimeUnit;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class ObjectStore {

    private final ImageReader imageReader;
    private final ImageWriter imageWriter;
    private ObjectStoreType type;

    public ObjectStore(DemoConfig demoConfig, ObjectStoreType type) {
        this.type = type;
        imageReader = makeImageReader(demoConfig, type, new BucketStats(RealDemoState.MAX_AGE_SECONDS, TimeUnit.SECONDS));
        imageWriter = makeImageWriter(demoConfig, type, new BucketStats(RealDemoState.MAX_AGE_SECONDS, TimeUnit.SECONDS));
    }

    public ImageReader getImageReader() {
        return imageReader;
    }

    public ImageWriter getImageWriter() {
        return imageWriter;
    }

    private ImageReader makeImageReader(DemoConfig demoConfig, ObjectStoreType type, BucketStats bucketStats) {
        switch (type) {
            case swift:
                return new SwiftImageReader(demoConfig.getAmHost(), demoConfig.getSwiftPort(), bucketStats);

            case amazonS3:
                return new S3ImageReader(demoConfig.getCreds(), bucketStats);

            default:
                return new S3ImageReader(demoConfig.getAmHost(), demoConfig.getS3Port(), bucketStats);
        }
    }

    private ImageWriter makeImageWriter(DemoConfig demoConfig, ObjectStoreType type, BucketStats bucketStats) {
        switch (type) {
            case swift:
                return new SwiftImageWriter(demoConfig.getAmHost(), demoConfig.getSwiftPort(), demoConfig.getVolumeNames(), bucketStats);

            case amazonS3:
                return new S3ImageWriter(demoConfig.getCreds(), demoConfig.getVolumeNames(), bucketStats);

            default:
                return new S3ImageWriter(demoConfig.getAmHost(), demoConfig.getS3Port(), demoConfig.getVolumeNames(), bucketStats);
        }
    }

    public ObjectStoreType getType() {
        return type;
    }
}
