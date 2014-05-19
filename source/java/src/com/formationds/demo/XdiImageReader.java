package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import org.apache.commons.io.IOUtils;

import java.io.InputStream;

public class XdiImageReader extends ImageReader {
    private Xdi xdi;

    public XdiImageReader(Xdi xdi) {
        super();
        this.xdi = xdi;
    }

    @Override
    public StoredImage read(StoredImage storedImage) throws Exception {
        Thread.sleep(500);
        InputStream inputStream = xdi.readStream(Main.DEMO_DOMAIN, storedImage.getVolumeName(), storedImage.getImageResource().getId());
        IOUtils.toByteArray(inputStream);
        increment(storedImage.getVolumeName());
        return storedImage;
    }
}

