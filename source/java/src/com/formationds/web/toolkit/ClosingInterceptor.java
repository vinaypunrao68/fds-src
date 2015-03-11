package com.formationds.web.toolkit;

import java.io.IOException;
import java.io.OutputStream;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class ClosingInterceptor extends OutputStream {
    private OutputStream underlying;

    public ClosingInterceptor(OutputStream underlying) {
        this.underlying = underlying;
    }

    @Override
    public void write(int b) throws IOException {
        underlying.write(b);
    }

    @Override
    public void write(byte[] b) throws IOException {
        underlying.write(b);
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        underlying.write(b, off, len);
    }

    @Override
    public void flush() throws IOException {
        underlying.flush();
    }

    @Override
    public void close() throws IOException {
    }

    // Because you don't want the stream closed twice
    public void doCloseForReal() throws IOException {
        underlying.close();
    }

}
