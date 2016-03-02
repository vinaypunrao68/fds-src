package com.formationds.index;

import com.formationds.nfs.Chunker;
import com.formationds.nfs.IoOps;

import java.io.IOException;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

public class ChunkedOutputStream extends OutputStream {
    private final String blobName;
    private final Chunker chunker;
    private long position;
    private IoOps io;
    private String luceneResourceName;
    private String domain;
    private String volume;
    private int objectSize;

    public ChunkedOutputStream(IoOps io, String luceneResourceName, String domain, String volume, String blobName, int objectSize) throws IOException {
        this.io = io;
        this.luceneResourceName = luceneResourceName;
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        chunker = new Chunker(io);
        this.blobName = blobName;
        Map<String, String> metadata = new HashMap<>();
        metadata.put(FdsLuceneDirectory.LUCENE_RESOURCE_NAME, luceneResourceName);
        metadata.put(FdsLuceneDirectory.SIZE, Long.toString(0l));
        io.writeMetadata(domain, volume, blobName, metadata);
        io.commitMetadata(domain, volume, blobName);
        this.position = 0;
    }

    @Override
    public void write(int b) throws IOException {
        write(new byte[]{(byte) b}, 0, 1);
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        byte[] buf = new byte[len];
        System.arraycopy(b, off, buf, 0, len);
        chunker.write(domain, volume, blobName, objectSize, buf, position, len, x -> null);
        position += len;
    }

    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    @Override
    public void flush() throws IOException {
        Map<String, String> metadata = new HashMap<>();
        metadata.put(FdsLuceneDirectory.SIZE, Long.toString(position));
        metadata.put(FdsLuceneDirectory.LUCENE_RESOURCE_NAME, luceneResourceName);
        io.writeMetadata(domain, volume, blobName, metadata);
        io.commitMetadata(domain, volume, blobName);
    }

    @Override
    public void close() throws IOException {
        flush();
    }
}
