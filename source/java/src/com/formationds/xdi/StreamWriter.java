package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ObjectOffset;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Map;

public class StreamWriter {

    private final byte[] buf;
    private AmService.Iface am;

    public StreamWriter(int objectSize, AmService.Iface am) {
        this.am = am;
        buf = new byte[objectSize];
    }

    public void write(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        long objectOffset = 0;
        int lastBufSize = 0;
        MessageDigest md = MessageDigest.getInstance("MD5");

        for (int read = in.read(buf); read != -1; read = in.read(buf)) {
            md.update(buf, 0, read);
            am.updateBlob(domainName, volumeName, blobName, ByteBuffer.wrap(buf, 0, read), read, new ObjectOffset(objectOffset), ByteBuffer.wrap(new byte[0]), false);
            lastBufSize = read;
            objectOffset++;
        }

        // do this until we have proper transactions
        if (lastBufSize != 0) {
            ByteBuffer digest = ByteBuffer.wrap(md.digest());
            am.updateBlob(domainName, volumeName, blobName, ByteBuffer.wrap(buf), lastBufSize, new ObjectOffset(objectOffset - 1), digest, true);
        }

        am.updateMetadata(domainName, volumeName, blobName, metadata);
    }
}
