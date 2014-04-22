package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class ImageResource {
    private String id;
    private String url;

    public ImageResource(String id, String url) {
        this.id = id;
        this.url = url;
    }

    public String getId() {
        return id;
    }

    public String getUrl() {
        return url;
    }
}
