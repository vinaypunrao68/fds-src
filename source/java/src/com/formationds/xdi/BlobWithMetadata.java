package com.formationds.xdi;

import com.formationds.protocol.BlobDescriptor;

import java.nio.ByteBuffer;

/**
* Copyright (c) 2014 Formation Data Systems, Inc.
*/
public class BlobWithMetadata {
    private ByteBuffer bytes;
    private BlobDescriptor blobDescriptor;

    public BlobWithMetadata(ByteBuffer bytes, BlobDescriptor blobDescriptor) {
        this.bytes = bytes;
        this.blobDescriptor = blobDescriptor;
    }

    public ByteBuffer getBytes() {
        return bytes;
    }

    public BlobDescriptor getBlobDescriptor() {
        return blobDescriptor;
    }
}
