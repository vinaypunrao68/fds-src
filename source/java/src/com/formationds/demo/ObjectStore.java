package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class ObjectStore {
    private DemoConfig demoConfig;
    private ObjectStoreType type;

    public ObjectStore(DemoConfig demoConfig, ObjectStoreType type) {
        this.demoConfig = demoConfig;
        this.type = type;
    }

    public ObjectStoreType getType() {
        return type;
    }

    public ImageReader getImageReader() {
        switch (type) {
            case swift:
                return new SwiftImageReader(demoConfig.getAmHost(), demoConfig.getSwiftPort());

            case amazonS3:
                return new S3ImageReader(demoConfig.getCreds());

            default:
                return new S3ImageReader(demoConfig.getAmHost(), demoConfig.getS3Port());
        }
    }

    public ImageWriter getImageWriter() {
        switch (type) {
            case swift:
                return new SwiftImageWriter(demoConfig.getAmHost(), demoConfig.getSwiftPort(), demoConfig.getVolumeNames());

            case amazonS3:
                return new S3ImageWriter(demoConfig.getCreds(), demoConfig.getVolumeNames());

            default:
                return new S3ImageWriter(demoConfig.getAmHost(), demoConfig.getS3Port(), demoConfig.getVolumeNames());
        }
    }
}
