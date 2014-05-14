package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Map;

public class StreamWriter {

    private int objectSize;
    private AmService.Iface am;

    public StreamWriter(int objectSize, AmService.Iface am) {
        this.objectSize = objectSize;
        this.am = am;
    }

    public byte[] write(String domainName, String volumeName, String blobName, InputStream input, Map<String, String> metadata) throws Exception {
        InputStream in = new BufferedInputStream(input, objectSize);
        long objectOffset = 0;
        int lastBufSize = 0;
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] digest = new byte[0];

        TxDescriptor tx = am.startBlobTx(domainName, volumeName, blobName);
        byte[] buf = new byte[objectSize];

        for (int read = readFully(in, buf); read != -1; read = readFully(in, buf)) {
            md.update(buf, 0, read);
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

        // am.updateMetadata(domainName, volumeName, blobName, tx, metadata);
        am.commitBlobTx(tx);
        return digest;
    }

    public int readFully(InputStream inputStream, byte[] buf) throws IOException {
        int bytesRead = 0;
        while (bytesRead < buf.length) {
            int read = inputStream.read(buf, bytesRead, buf.length - bytesRead);
            if (read == -1) break;
            bytesRead += read;
        }
        return bytesRead == 0 ? -1 : bytesRead;
    }

}
