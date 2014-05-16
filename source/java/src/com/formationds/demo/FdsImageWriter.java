package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import com.google.common.collect.Lists;
import org.apache.thrift.TException;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.net.URL;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class FdsImageWriter implements ImageWriter {
    public static final String URL_METADATA = "url";
    public static final String ID_METADATA = "id";

    private Counts counts;
    private Xdi xdi;
    private final String[] volumeNames;

    public FdsImageWriter(Xdi xdi) {
        try {
            counts = new Counts();
            this.xdi = xdi;
            volumeNames = xdi.listVolumes(Main.DEMO_DOMAIN).stream().map(d -> d.getName()).toArray(i -> new String[i]);
        } catch (TException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public Counts consumeCounts() {
        return counts.consume();
    }

    @Override
    public StoredImage write(ImageResource resource) {
        try {
            Thread.sleep(500);
            String volume = randomVolume();
            URL url = new URL(resource.getUrl());
            InputStream inputStream = new BufferedInputStream(url.openConnection().getInputStream(), 1024 * 10);
            Map<String, String> metadata = new HashMap<>();
            metadata.put(URL_METADATA, resource.getUrl());
            metadata.put(ID_METADATA, resource.getId());
            xdi.writeStream(Main.DEMO_DOMAIN, volume, resource.getId(), inputStream, metadata);
            counts.increment(volume);
            return new StoredImage(resource, volume);
        } catch (Exception e) {
            System.out.println(e);
            throw new RuntimeException(e);
        }
    }

    private String randomVolume() {
        List<String> l = Lists.newArrayList(volumeNames);
        Collections.shuffle(l);
        return l.get(0);
    }
}
