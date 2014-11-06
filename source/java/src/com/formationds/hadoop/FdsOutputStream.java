package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.util.blob.Mode;
import com.formationds.xdi.FdsObjectFrame;
import org.apache.thrift.TException;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;

public class FdsOutputStream extends OutputStream {
    private final int objectSize;
    private AmService.Iface am;
    private final String domain;
    private final String volume;
    private final String blobName;
    private long currentOffset;

    private long currentObject;
    private ByteBuffer currentBuffer;
    private boolean isDirty;

    private FdsOutputStream(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        this.am = am;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.currentOffset = 0L;
        this.objectSize = objectSize;
    }

    public static FdsOutputStream openNew(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        FdsOutputStream fdsOutputStream = new FdsOutputStream(am, domain, volume, blobName, objectSize);
        try {
            am.updateBlobOnce(domain, volume, blobName, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), makeMetadata());
            return fdsOutputStream;
        } catch(Exception ex) {
            throw new IOException(ex);
        }
    }

    public static FdsOutputStream openForAppend(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        FdsOutputStream fdsOutputStream = new FdsOutputStream(am, domain, volume, blobName, objectSize);
        try {
            BlobDescriptor bd = am.statBlob(domain, volume, blobName);
            fdsOutputStream.currentOffset = bd.getByteCount();
            return fdsOutputStream;
        } catch(Exception ex) {
            throw new IOException(ex);
        }
    }

    public void flush() throws IOException {
        try {
            if (currentBuffer != null && isDirty) {
                Map<String, String> metadata = makeMetadata();

                int position = currentBuffer.position();

                if (currentBuffer.remaining() == 0) {
                    currentBuffer.flip();
                    am.updateBlobOnce(domain, volume, blobName, Mode.TRUNCATE.getValue(), currentBuffer, currentBuffer.limit(), new ObjectOffset(currentObject), metadata);
                } else {
                    // read-update-write
                    ByteBuffer existing = null;
                    try {
                        existing = am.getBlob(domain, volume, blobName, objectSize, new ObjectOffset(currentObject));
                    } catch(ApiException ex) {
                        if(ex.getErrorCode() == ErrorCode.MISSING_RESOURCE)
                            existing = ByteBuffer.allocate(0);
                        else
                            throw ex;
                    }
                    if(existing.limit() > currentBuffer.limit()) {
                        existing.position(currentBuffer.limit());
                        currentBuffer.position(currentBuffer.limit());
                        currentBuffer.limit(existing.limit());
                        currentBuffer.put(existing);
                    }
                    currentBuffer.flip();
                    am.updateBlobOnce(domain, volume, blobName, Mode.TRUNCATE.getValue(), currentBuffer, currentBuffer.limit(), new ObjectOffset(currentObject), metadata);
                }

                currentBuffer.position(position);
                currentBuffer.limit(currentBuffer.capacity());
                isDirty = false;
            }
        } catch(Exception e) {
            throw new IOException(e);
        }
    }

    private static Map<String, String> makeMetadata() {
        HashMap<String, String> md = new HashMap<>();
        md.put(FdsFileSystem.LAST_MODIFIED_KEY, Long.toString(Calendar.getInstance().getTimeInMillis()));
        return md;
    }

    private ByteBuffer getBuffer() throws IOException {
        FdsObjectFrame frame = FdsObjectFrame.firstFrame(currentOffset, 1, objectSize);
        if(frame.objectOffset != currentObject || currentBuffer == null) {
            flush();

            currentObject = frame.objectOffset;
            currentBuffer = ByteBuffer.allocate(objectSize);
            if(frame.internalOffset > 0) {
                try {
                    ByteBuffer buffer = am.getBlob(domain, volume, blobName, frame.internalOffset, new ObjectOffset(currentObject));
                    currentBuffer.put(buffer);
                } catch(TException ex) {
                    if(!(ex instanceof ApiException) || ((ApiException) ex).errorCode == ErrorCode.MISSING_RESOURCE) {
                        throw new IOException(ex);
                    }
                }
            }

            isDirty = false;
        }
        currentBuffer.position(frame.internalOffset);
        return currentBuffer;
    }

    @Override
    public void write(int b) throws IOException {
        getBuffer().put((byte)(b % 256));
        isDirty = true;
        currentOffset++;
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        int written = 0;
        while(written < len) {
            ByteBuffer buffer = getBuffer();
            int writeSize = Math.min(buffer.remaining(), len - written);
            buffer.put(b, off + written, writeSize);
            written += writeSize;
            currentOffset += writeSize;
            isDirty = true;
        }
    }

    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    @Override
    public void close() throws IOException {
        flush();
        super.close();
    }
}
