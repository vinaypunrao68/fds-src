package com.formationds.index;

import com.formationds.nfs.Chunker;
import com.formationds.nfs.FdsMetadata;
import com.formationds.nfs.IoOps;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Map;
import java.util.Optional;

public class ChunkedOutputStream extends OutputStream {
    private final String blobName;
    private final Chunker chunker;
    private long position;
    private IoOps io;
    private String domain;
    private String volume;
    private int objectSize;

    public ChunkedOutputStream(IoOps io, String luceneResourceName, String domain, String volume, String blobName, int objectSize) throws IOException {
        this.io = io;
        this.domain = domain;
        this.volume = volume;
        this.objectSize = objectSize;
        chunker = new Chunker(io);
        this.blobName = blobName;
        this.position = io.readMetadata(domain, volume, blobName).orElse(new FdsMetadata()).lock(m -> {
            Map<String, String> metadata = m.mutableMap();
            if (metadata.size() == 0) {
                metadata.put(FdsLuceneDirectory.LUCENE_RESOURCE_NAME, luceneResourceName);
                metadata.put(FdsLuceneDirectory.SIZE, Long.toString(0l));
            }

            io.writeMetadata(domain, volume, blobName, m.fdsMetadata());
            io.commitMetadata(domain, volume, blobName);
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
        chunker.write(domain, volume, blobName, objectSize, buf, position, len, x -> null);
        position += len;
    }

    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    @Override
    public void flush() throws IOException {
        Optional<FdsMetadata> opt = io.readMetadata(domain, volume, blobName);
        if (!opt.isPresent()) {
            throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
        }
        opt.get().lock(m -> {
            m.mutableMap().put(FdsLuceneDirectory.SIZE, Long.toString(position));
            io.writeMetadata(domain, volume, blobName, m.fdsMetadata());
            io.commitMetadata(domain, volume, blobName);
            return null;
        });
    }

    @Override
    public void close() throws IOException {
        flush();
    }
}
