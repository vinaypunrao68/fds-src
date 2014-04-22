package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import com.formationds.xdi.shim.BlobDescriptor;
import com.formationds.xdi.shim.VolumeDescriptor;
import org.apache.commons.io.IOUtils;

import java.io.InputStream;
import java.util.Collections;
import java.util.List;
import java.util.Map;

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
    public ImageResource readOne() {
        try {
            List<VolumeDescriptor> volumes = xdi.listVolumes(Main.DEMO_DOMAIN);
            Collections.shuffle(volumes);
            String volumeName = volumes.get(0).getName();
            List<BlobDescriptor> blobs = xdi.volumeContents(Main.DEMO_DOMAIN, volumeName, 1000, 0l);
            Collections.shuffle(blobs);
            BlobDescriptor blobDescriptor = blobs.get(0);
            Map<String, String> metadata = blobDescriptor.getMetadata();
            String id = metadata.get(FdsImageWriter.ID_METADATA);
            String url = metadata.get(FdsImageWriter.URL_METADATA);
            InputStream inputStream = xdi.readStream(Main.DEMO_DOMAIN, volumeName, blobDescriptor.getName());
            byte[] bytes = IOUtils.toByteArray(inputStream);
            counts.increment(volumeName);
            return new ImageResource(id, url, bytes.length);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
