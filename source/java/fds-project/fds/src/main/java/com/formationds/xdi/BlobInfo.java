package com.formationds.xdi;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.BlobDescriptor;

import java.nio.ByteBuffer;

public class BlobInfo {
    private String domain;
    private String volume;
    private String blob;
    private BlobDescriptor blobDescriptor;
    private VolumeDescriptor volumeDescriptor;
    private ByteBuffer object0;

    public BlobInfo(String domain, String volume, String blob, BlobDescriptor bd, VolumeDescriptor vd, ByteBuffer object0) {
        this.domain = domain;
        this.volume = volume;
        this.blob = blob;
        blobDescriptor = bd;
        volumeDescriptor = vd;
        this.object0 = object0;
    }

    public String getDomain() {
        return domain;
    }

    public String getVolume() {
        return volume;
    }

    public String getBlob() {
        return blob;
    }

    public BlobDescriptor getBlobDescriptor() {
        return blobDescriptor;
    }

    public VolumeDescriptor getVolumeDescriptor() {
        return volumeDescriptor;
    }

    public ByteBuffer getObject0() {
        return object0;
    }
}
