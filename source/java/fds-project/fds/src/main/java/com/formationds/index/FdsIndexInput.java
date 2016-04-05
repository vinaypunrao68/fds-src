package com.formationds.index;

import com.formationds.nfs.Chunker;
import com.formationds.nfs.IoOps;
import org.apache.lucene.store.IndexInput;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Map;
import java.util.Optional;

public class FdsIndexInput extends IndexInput {
    private Chunker chunker;
    private final long blobLength;
    private final long logicalLength;
    private final long offset;
    private long position;
    private String domain;
    private String volume;
    private String blobName;
    private int objectSize;
    private IoOps io;

    public FdsIndexInput(Chunker chunker, IoOps io, String resourceName, String domain, String volume, String blobName, int objectSize) throws IOException {
        super(resourceName);
        this.io = io;
        this.chunker = chunker;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectSize = objectSize;
        this.offset = 0;
        this.position = 0;
        Optional<Map<String, String>> opt = io.readMetadata(domain, volume, blobName);
        if (!opt.isPresent()) {
            throw new FileNotFoundException("Volume=" + volume + ", blobName=" + blobName);
        }
        this.blobLength = Long.parseLong(opt.get().get(FdsLuceneDirectory.SIZE));
        this.logicalLength = blobLength;
    }

    private FdsIndexInput(Chunker chunker, IoOps io, String resourceName, String domain, String volume, String blobName, int objectSize, long offset, long blobLength, long logicalLength) {
        super(resourceName);
        this.io = io;
        this.chunker = chunker;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.objectSize = objectSize;
        this.offset = offset;
        this.position = offset;
        this.blobLength = blobLength;
        this.logicalLength = logicalLength;
    }

    @Override
    public IndexInput clone() {
        FdsIndexInput indexInput = new FdsIndexInput(chunker, io, toString(), domain, volume, blobName, objectSize, offset, blobLength, logicalLength);
        indexInput.position = this.position;
        return indexInput;
    }

    @Override
    public void close() throws IOException {
    }

    @Override
    public long getFilePointer() {
        return position - offset;
    }

    @Override
    public void seek(long l) throws IOException {
        position = l + offset;
    }

    @Override
    public long length() {
        return logicalLength;
    }

    @Override
    public IndexInput slice(String name, long offset, long length) throws IOException {
        return new FdsIndexInput(chunker, io, name, domain, volume, blobName, objectSize, offset + this.offset, blobLength, length);
    }

    @Override
    public byte readByte() throws IOException {
        byte[] buf = new byte[1];
        readBytes(buf, 0, 1);
        return buf[0];
    }

    @Override
    public void readBytes(byte[] bytes, int offset, int length) throws IOException {
        byte[] buf = new byte[length];
        chunker.read(domain, volume, blobName, blobLength, objectSize, buf, position, length);
        position += length;
        System.arraycopy(buf, 0, bytes, offset, length);
    }
}
