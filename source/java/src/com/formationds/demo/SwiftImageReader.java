package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.io.IOUtils;

import java.io.InputStream;
import java.net.URL;
import java.text.MessageFormat;

public class SwiftImageReader extends ImageReader {
    public static final String URL_PATTERN = "http://{0}:{1}/v1/FDS/{2}/{3}";
    private String host;
    private int port;

    public SwiftImageReader(String host, int port, BucketStats bucketStats) {
        super(bucketStats);
        this.host = host;
        this.port = port;
    }

    @Override
    protected StoredImage read(StoredImage storedImage) throws Exception {
        String url = MessageFormat.format(URL_PATTERN, host, Integer.toString(port), storedImage.getVolumeName(), storedImage.getImageResource().getName());
        try (InputStream inputStream = new URL(url).openConnection().getInputStream()) {
            IOUtils.toByteArray(inputStream);
            increment(storedImage.getVolumeName());
        }
        return storedImage;

    }
}
