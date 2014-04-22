package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class ImageResource {
    private String id;
    private String url;
    private int sizeInBytes;

    public ImageResource(String id, String url) {
        this(id, url, 0);
    }

    public ImageResource(String id, String url, int sizeInBytes) {
        this.id = id;
        this.url = url;
        this.sizeInBytes = sizeInBytes;
    }

    public String getId() {
        return id;
    }

    public String getUrl() {
        return url;
    }
}
