package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import com.google.common.collect.Maps;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.net.URL;

public class XdiImageWriter extends ImageWriter {
    private Xdi xdi;

    public XdiImageWriter(Xdi xdi, String[] volumeNames, BucketStats bucketStats) {
        super(volumeNames, bucketStats);
        this.xdi = xdi;
    }

    @Override
    public StoredImage write(ImageResource resource) {
        try {
            String volume = randomVolume();
            URL url = new URL(resource.getUrl());
            try (InputStream inputStream = new BufferedInputStream(url.openConnection().getInputStream(), 1024 * 10)) {
                xdi.writeStream(Main.DEMO_DOMAIN, volume, resource.getId(), inputStream, Maps.newHashMap());
                increment(volume);
                return new StoredImage(resource, volume);
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

}
