package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.ObjectOffset;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Map;

public class StreamWriter {

    private final byte[] buf;
    private AmShim.Iface am;

    public StreamWriter(int objectSize, AmShim.Iface am) {
        this.am = am;
        buf = new byte[objectSize];
    }

    public void write(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        long objectOffset = 0;
        int lastBufSize = 0;

        for (int read = in.read(buf); read != -1; read = in.read(buf)) {
            am.updateBlob(domainName, volumeName, blobName, ByteBuffer.wrap(buf, 0, read), read, new ObjectOffset(objectOffset), false);
            lastBufSize = read;
            objectOffset++;
        }

        // do this until we have proper transactions
        if (lastBufSize != 0) {
            am.updateBlob(domainName, volumeName, blobName, ByteBuffer.wrap(buf), lastBufSize, new ObjectOffset(objectOffset - 1), true);
        }

        am.updateMetadata(domainName, volumeName, blobName, metadata);
    }
}
