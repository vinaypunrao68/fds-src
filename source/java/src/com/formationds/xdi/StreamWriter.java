package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ObjectOffset;
import com.formationds.apis.TxDescriptor;
import org.apache.commons.codec.binary.Hex;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Map;

public class StreamWriter {
    private int objectSize;
    private AmService.Iface am;

    private static ThreadLocal<byte[]> localBytes = new ThreadLocal<>();

    public StreamWriter(int objectSize, AmService.Iface am) {
        this.objectSize = objectSize;
        this.am = am;
    }

    public byte[] write(String domainName, String volumeName, String blobName, InputStream input, Map<String, String> metadata) throws Exception {
        long objectOffset = 0;
        int lastBufSize = 0;
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] digest = new byte[0];

        TxDescriptor tx = am.startBlobTx(domainName, volumeName, blobName);
        if (localBytes.get() == null || localBytes.get().length != objectSize) {
            localBytes.set(new byte[objectSize]);
        }

        byte[] buf = localBytes.get();

        for (int read = readFully(input, buf); read != -1; read = readFully(input, buf)) {
            md.update(buf, 0, read);
            am.updateBlob(domainName, volumeName, blobName, tx,
                    ByteBuffer.wrap(buf, 0, read), read,
                    new ObjectOffset(objectOffset), false);
            lastBufSize = read;
            objectOffset++;
        }

        // do this until we have proper transactions
        if (lastBufSize != 0) {
            am.updateBlob(domainName, volumeName, blobName, tx,
                    ByteBuffer.wrap(buf), lastBufSize,
                    new ObjectOffset(objectOffset - 1), true);
        }

        digest = md.digest();
        metadata.put("etag", Hex.encodeHexString(digest));

        am.updateMetadata(domainName, volumeName, blobName, tx, metadata);
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
