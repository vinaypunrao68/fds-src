package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.io.IOUtils;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.MessageFormat;

public class SwiftImageWriter extends ImageWriter {
    private String host;
    private int port;

    public SwiftImageWriter(String host, int port, String[] volumeNames) {
        super(volumeNames);
        this.host = host;
        this.port = port;
    }

    @Override
    public StoredImage write(ImageResource resource) {
        try {
            String volume = randomVolume();
            URL sourceUrl = new URL(resource.getUrl());
            try (InputStream inputStream = new BufferedInputStream(sourceUrl.openConnection().getInputStream(), 1024 * 10)) {
                String dest = MessageFormat.format(SwiftImageReader.URL_PATTERN, host, Integer.toString(port), volume, resource.getId());
                HttpURLConnection cnx = (HttpURLConnection) new URL(dest).openConnection();
                cnx.setDoOutput(true);
                cnx.setRequestMethod("PUT");
                try (OutputStream outputStream = cnx.getOutputStream()) {
                    IOUtils.copy(inputStream, outputStream);
                    outputStream.close();
                    inputStream.close();
                }
                increment(volume);
            }

            return new StoredImage(resource, volume);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
