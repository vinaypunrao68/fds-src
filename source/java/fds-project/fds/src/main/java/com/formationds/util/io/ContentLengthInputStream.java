package com.formationds.util.io;

import com.formationds.commons.NotImplementedException;

import java.io.IOException;
import java.io.InputStream;

public class ContentLengthInputStream extends InputStream {
    private final InputStream wrapped;
    private final long contentLength;
    private long remaining;

    public ContentLengthInputStream(InputStream inputStream, long contentLength) {
        this.wrapped = inputStream;
        this.contentLength = contentLength;
        this.remaining = contentLength;
    }

    @Override
    public int available() throws IOException {
        return wrapped.available();
    }

    @Override
    public long skip(long n) throws IOException {
        long skipped = wrapped.skip(n);
        remaining -= skipped;
        return skipped;
    }

    @Override
    public void close() throws IOException {
        wrapped.close();
    }

    @Override
    public boolean markSupported() {
        return false;
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
    public int read(byte[] b, int off, int len) throws IOException {
        int result = wrapped.read(b, off, len);
        processRead(result);
        return result;
    }

    @Override
    public int read(byte[] b) throws IOException {
        int result = wrapped.read(b);
        processRead(result);
        return result;
    }

    @Override
    public int read() throws IOException {
        int result = wrapped.read();
        processRead(result == -1 ? -1 : 1);
        return result;
    }

    private void processRead(int readLength) throws IOException {
        if(readLength > 0) {
            remaining -= readLength;
        } else {
            if(remaining > 0) {
                String message = String.format("Stream ended before all expected content was received based on expected content length [got %d/%d bytes]", contentLength - remaining, contentLength);
                throw new NotAllContentReceivedException(message);
            }
        }
    }

    public class NotAllContentReceivedException extends IOException {
        public NotAllContentReceivedException(String s) {
            super(s);
        }
    }
}
