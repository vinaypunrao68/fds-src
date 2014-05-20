package com.formationds.demo;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class ObjectStore {

    private final ImageReader imageReader;
    private final ImageWriter imageWriter;
    private ObjectStoreType type;

    public ObjectStore(DemoConfig demoConfig, ObjectStoreType type) {
        this.type = type;
        imageReader = makeImageReader(demoConfig, type);
        imageWriter = makeImageWriter(demoConfig, type);
    }

    public ImageReader getImageReader() {
        return imageReader;
    }

    public ImageWriter getImageWriter() {
        return imageWriter;
    }

    private ImageReader makeImageReader(DemoConfig demoConfig, ObjectStoreType type) {
        switch (type) {
            case swift:
                return new SwiftImageReader(demoConfig.getAmHost(), demoConfig.getSwiftPort());

            case amazonS3:
                return new S3ImageReader(demoConfig.getCreds());

            default:
                return new S3ImageReader(demoConfig.getAmHost(), demoConfig.getS3Port());
        }
    }

    private ImageWriter makeImageWriter(DemoConfig demoConfig, ObjectStoreType type) {
        switch (type) {
            case swift:
                return new SwiftImageWriter(demoConfig.getAmHost(), demoConfig.getSwiftPort(), demoConfig.getVolumeNames());

            case amazonS3:
                return new S3ImageWriter(demoConfig.getCreds(), demoConfig.getVolumeNames());

            default:
                return new S3ImageWriter(demoConfig.getAmHost(), demoConfig.getS3Port(), demoConfig.getVolumeNames());
        }
    }

    public ObjectStoreType getType() {
        return type;
    }
}
