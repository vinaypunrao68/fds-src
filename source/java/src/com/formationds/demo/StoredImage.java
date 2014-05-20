package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class StoredImage {
    private ImageResource imageResource;
    private String volumeName;

    public StoredImage(ImageResource imageResource, String volumeName) {
        this.imageResource = imageResource;
        this.volumeName = volumeName;
    }

    public ImageResource getImageResource() {
        return imageResource;
    }

    public String getVolumeName() {
        return volumeName;
    }
}
