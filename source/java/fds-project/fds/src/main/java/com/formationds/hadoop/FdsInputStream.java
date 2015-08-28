package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.FdsObjectFrame;
import org.apache.hadoop.fs.PositionedReadable;
import org.apache.hadoop.fs.Seekable;
import org.apache.log4j.Logger;

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class FdsInputStream extends InputStream implements Seekable, PositionedReadable {
    private static final Logger LOG = Logger.getLogger(FdsInputStream.class);
    private final int objectSize;
    private final String domain;
    private final String volume;
    private final String blobName;
    private final Object bufferStateLock = new Object();
    private AsyncAm asyncAm;
    private long readObject;
    private ByteBuffer readBuffer;
    private long position;

    public FdsInputStream(AsyncAm am, String domain, String volume, String blobName, int objectSize) {
        this.asyncAm = am;
        this.objectSize = objectSize;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
    }

    private ByteBuffer getBuffer(long offset) throws IOException {
        FdsObjectFrame frame = FdsObjectFrame.firstFrame(offset, 1, objectSize);
        try {
            ByteBuffer objectBuffer = null;

            synchronized (bufferStateLock) {
                if (frame.objectOffset == readObject) {
                    objectBuffer = this.readBuffer;
                }
            }

            if(objectBuffer == null) {
                objectBuffer = unwindExceptions(() -> asyncAm.getBlob(domain, volume, blobName, objectSize, new ObjectOffset(frame.objectOffset)).get());
                synchronized (bufferStateLock) {
                    readObject = frame.objectOffset;
                    this.readBuffer = objectBuffer;
                }
            }
            ByteBuffer retval = readBuffer.slice();
            retval.position(Math.min(retval.position() + frame.internalOffset, retval.limit()));
            return retval;
        } catch(ApiException ex) {
            if(ex.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                return ByteBuffer.allocate(0);
            LOG.error("AM: getBlob() error", ex);
            throw new IOException("Read failed", ex);
        } catch(Exception ex) {
            LOG.error("AM: getBlob() error", ex);
            throw new IOException("Read failed", ex);
        }
    }

    @Override
    public synchronized int read(byte[] b, int off, int len) throws IOException {
        int readLength = read(position, b, off, len);
        position += readLength;
        return readLength;
    }

    public long length() throws Exception {
        long byteCount = 0;
        try {
            byteCount = unwindExceptions(() -> asyncAm.statBlob(domain, volume, blobName).get()).getByteCount();
        } catch (ApiException e) {
            if (e.getErrorCode().equals(ErrorCode.MISSING_RESOURCE)) {
                byteCount = 0;
            } else {
                LOG.error("AM: statBlob(" + blobName + ") error", e);
                throw e;
            }
        } catch (Exception e) {
            LOG.error("AM: statBlob(" + blobName + ") error", e);
            throw e;
        }
        return Math.max(position, byteCount);
    }

    @Override
    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public synchronized int read() throws IOException {
        ByteBuffer buf = getBuffer(position);
        if(buf.remaining() == 0)
            return -1;
        position++;

        return Byte.toUnsignedInt(buf.get());
    }

    @Override
    public synchronized void seek(long pos) throws IOException {
        position = pos;
    }

    @Override
    public synchronized long getPos() throws IOException {
        return position;
    }

    // TODO: semantics of this?
    @Override
    public boolean seekToNewSource(long targetPos) throws IOException {
        return false;
    }

    @Override
    public int read(long position, byte[] buffer, int offset, int length) throws IOException {
        ByteBuffer data = getBuffer(position);
        if(length == 0)
            return 0;

        int readLength = Math.min(length, data.remaining());
        if(readLength == 0)
            return -1;
        data.get(buffer, offset, readLength);
        return readLength;
    }

    @Override
    public void readFully(long position, byte[] buffer, int offset, int length) throws IOException {
        int bytesRead = 0;
        while(bytesRead < length) {
            int readLength = read(position + bytesRead, buffer, offset + bytesRead, length - bytesRead);
            if(readLength == -1)
                throw new EOFException();
            bytesRead += readLength;
        }
    }

    @Override
    public void readFully(long position, byte[] buffer) throws IOException {
        readFully(position, buffer, 0, buffer.length);
    }

    public String getBlobName() {
        return blobName;
    }
}
