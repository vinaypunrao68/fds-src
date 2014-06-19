package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class FlickrImageResource implements ImageResource {
    private static String LARGE_IMAGE_URL = "http://farm%d.staticflickr.com/%s/%s_%s_z.jpg";
    private static String THUMBNAIL_IMAGE_URL = "http://farm%d.staticflickr.com/%s/%s_%s_m.jpg";

    private String id;
    private int farm;
    private String server;
    private String secret;

    public FlickrImageResource(String id, int farm, String server, String secret) {
        this.id = id;
        this.farm = farm;
        this.server = server;
        this.secret = secret;

    }

    @Override
    public String getId() {
        return id;
    }

    @Override
    public String getName() {
        return id + ".jpg";
    }

    @Override
    public String getUrl() {
        return String.format(LARGE_IMAGE_URL, farm, server, id, secret);
    }

    @Override
    public String getThumbnailUrl() {
        return String.format(THUMBNAIL_IMAGE_URL, farm, server, id, secret);
    }
}
