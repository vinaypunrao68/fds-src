package com.formationds.util;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

public class ChunkingInputStream extends InputStream {
    private InputStream stream;
    private int size;

    public ChunkingInputStream(InputStream stream, int size) {
        this.stream = stream;
        this.size = size;
    }

    @Override
    public int read() throws IOException {
        return stream.read();
    }

    @Override
    public int read(byte[] b) throws IOException {
        return stream.read(b, 0, size);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        return stream.read(b, off, Math.min(size, len));
    }

    @Override
    public void close() throws IOException {
        super.close();
    }

    @Override
    public boolean markSupported() {
        return false;
    }
}
