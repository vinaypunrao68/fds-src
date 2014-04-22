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
import java.util.*;

public class FdsImageWriter implements ImageWriter {
    private Counts counts;
    private Xdi xdi;
    private final String[] volumeNames;

    public FdsImageWriter(Xdi xdi) throws TException {
        counts = new Counts();
        this.xdi = xdi;
        volumeNames = xdi.listVolumes(Main.DEMO_DOMAIN).stream().map(d -> d.getName()).toArray(i -> new String[i]);
    }

    @Override
    public Counts consumeCounts() {
        return counts.consume();
    }

    @Override
    public void write(ImageResource resource) {
        try {
            String volume = randomVolume();
            URL url = new URL(resource.getUrl());
            InputStream inputStream = new BufferedInputStream(url.openConnection().getInputStream(), 1024 * 10);
            Map<String, String> metadata = new HashMap<>();
            metadata.put("url", resource.getUrl());
            metadata.put("id", resource.getId());
            String name = UUID.randomUUID().toString() + ".jpg";
            xdi.writeStream(Main.DEMO_DOMAIN, volume, name, inputStream, metadata);
            counts.increment(volume);
        } catch (Exception e) {
            System.out.println(e);
        }
    }

    private String randomVolume() {
        List<String> l = Lists.newArrayList(volumeNames);
        Collections.shuffle(l);
        return l.get(0);
    }
}
