package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;

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

    public byte[] write(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        long objectOffset = 0;
        int lastBufSize = 0;
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] digest = new byte[0];

        TxDescriptor tx = am.startBlobTx(domainName, volumeName, blobName);

        for (int read = in.read(buf); read != -1; read = in.read(buf)) {
            md.update(buf, 0, read);
            // Just create a fake tx id for now to make the compiler happy...
            am.updateBlob(domainName, volumeName, blobName, tx,
                          ByteBuffer.wrap(buf, 0, read), read,
                          new ObjectOffset(objectOffset),
                          ByteBuffer.wrap(new byte[0]), false);
            lastBufSize = read;
            objectOffset++;
        }

        // do this until we have proper transactions
        if (lastBufSize != 0) {
            digest = md.digest();
            ByteBuffer byteBuffer = ByteBuffer.wrap(digest);
            am.updateBlob(domainName, volumeName, blobName, tx,
                          ByteBuffer.wrap(buf), lastBufSize,
                          new ObjectOffset(objectOffset - 1),
                          byteBuffer, true);
        }

        am.updateMetadata(domainName, volumeName, blobName, tx, metadata);
        am.commitBlobTx(tx);
        return digest;
    }
}
