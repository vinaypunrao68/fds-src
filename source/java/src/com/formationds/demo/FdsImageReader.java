package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import org.apache.commons.io.IOUtils;

import java.io.InputStream;

public class FdsImageReader implements ImageReader {
    private Counts counts;
    private Xdi xdi;

    public FdsImageReader(Xdi xdi) {
        this.xdi = xdi;
        this.counts = new Counts();
    }

    @Override
    public Counts consumeCounts() {
        return counts.consume();
    }

    @Override
    public StoredImage read(StoredImage lastWritten) {
        try {
            Thread.sleep(500);
            InputStream inputStream = xdi.readStream(Main.DEMO_DOMAIN, lastWritten.getVolumeName(), lastWritten.getImageResource().getId());
            IOUtils.toByteArray(inputStream);
            counts.increment(lastWritten.getVolumeName());
            return lastWritten;
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
