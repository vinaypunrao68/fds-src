package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.shim.AmShim;
import com.formationds.xdi.shim.Uuid;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Map;

public class StreamWriter {

    private final byte[] buf;
    private AmShim.Iface am;

    public StreamWriter(int bufSize, AmShim.Iface am) {
        this.am = am;
        buf = new byte[bufSize];
    }

    public void write(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        Uuid txId = am.startBlobTx(domainName, volumeName, blobName);
        long offset = 0;
        for (int read = in.read(buf); read != -1; read = in.read(buf)) {
            am.updateBlob(domainName, volumeName, blobName, txId, ByteBuffer.wrap(buf, 0, read), read, offset);
            offset += read;
        }
        am.updateMetadata(domainName, volumeName, blobName, txId, metadata);
        am.commit(txId);
    }
}
