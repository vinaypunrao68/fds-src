package com.formationds.xdi.io;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

public class ByteBufferInputStream extends InputStream {
    public ByteBuffer buffer;

    public ByteBufferInputStream(ByteBuffer buffer) {
        this.buffer = buffer;
    }

    @Override
    public int read() throws IOException {
        if(buffer.remaining() == 0)
            return -1;

        return buffer.get();
    }

    @Override
    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        if(buffer.remaining() == 0)
            return -1;

        int read = Math.min(len, buffer.remaining());
        buffer.get(b, off, read);
        return read;
    }
}
