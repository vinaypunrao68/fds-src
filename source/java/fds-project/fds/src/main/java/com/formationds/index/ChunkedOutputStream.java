package com.formationds.index;

import com.formationds.nfs.Chunker;
import com.formationds.nfs.TransactionalIo;

import java.io.IOException;
import java.io.OutputStream;

public class ChunkedOutputStream extends OutputStream {
    private final String blobName;
    private final Chunker chunker;
    private long position;
    private String domain;
    private String volume;
    private int objectSize;

    public ChunkedOutputStream(TransactionalIo io, String domain, String volume, String blobName, int objectSize) throws IOException {
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        chunker = new Chunker(io);
        this.blobName = blobName;
        position = io.mutateMetadata(domain, volume, blobName, false, metadata -> {
            if (metadata.size() == 0) {
                metadata.put(FdsLuceneDirectory.NAME, blobName);
                metadata.put(FdsLuceneDirectory.SIZE, Long.toString(0l));
            }
            return 0l;
        });
    }

    @Override
    public void write(int b) throws IOException {
        write(new byte[]{(byte) b}, 0, 1);
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        byte[] buf = new byte[len];
        System.arraycopy(b, off, buf, 0, len);
        chunker.write(domain, volume, blobName, objectSize, buf, position, len, metadata -> {
            position += len;
            metadata.put(FdsLuceneDirectory.SIZE, Long.toString(position));
            return null;
        });
    }

    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }
}
