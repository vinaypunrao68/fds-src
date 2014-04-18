package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import org.apache.commons.lang.NotImplementedException;

import java.io.IOException;
import java.io.InputStream;
import java.util.Iterator;

public class BlockStreamer extends InputStream {
    private Iterator<byte[]> sources;
    private byte[] current;
    private int offset;

    public BlockStreamer(Iterator<byte[]> blocks) {
        this.sources = blocks;
    }

    @Override
    public int read() throws IOException {
        byte[] buf = new byte[1];
        int readBytes = read(buf);
        return readBytes == -1 ? readBytes : buf[0];
    }

    @Override
    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        if (refreshIfNeeded() == -1) {
            return -1;
        }
        int bytesToRead = Math.min(len, current.length - offset);
        System.arraycopy(current, offset, b, off, bytesToRead);
        offset += bytesToRead;
        return bytesToRead;
    }

    private int refreshIfNeeded() {
        if (current == null || offset >= current.length) {
            if (!sources.hasNext()) {
                return -1;
            }
            current = sources.next();
            offset = 0;
        }

        return current.length - offset;
    }
    @Override
    public long skip(long n) throws IOException {
        throw new NotImplementedException();
    }

    @Override
    public int available() throws IOException {
        return refreshIfNeeded();
    }

    @Override
    public void close() throws IOException {
        sources = null;
        current = null;
    }

    @Override
    public synchronized void mark(int readlimit) {
        throw new NotImplementedException();
    }

    @Override
    public synchronized void reset() throws IOException {
        throw new NotImplementedException();
    }

    @Override
    public boolean markSupported() {
        return false;
    }
}
