package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import org.apache.thrift.TException;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;

public class FdsOutputStream extends OutputStream {
    private final int objectSize;
    private final String domain;
    private final String volume;
    private final String blobName;
    private AmService.Iface am;
    private long currentOffset;
    private ByteBuffer currentBuffer;
    private boolean isDirty;
    private boolean isClosed;

    FdsOutputStream(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        this(objectSize, domain, volume, blobName, am, 0, ByteBuffer.allocate(objectSize), false, false);
    }

    private FdsOutputStream(int objectSize, String domain, String volume, String blobName, AmService.Iface am, long currentOffset, ByteBuffer currentBuffer, boolean isDirty, boolean isClosed) {
        this.objectSize = objectSize;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.am = am;
        this.currentOffset = currentOffset;
        this.currentBuffer = currentBuffer;
        this.isDirty = isDirty;
        this.isClosed = isClosed;
    }

    public static FdsOutputStream openForAppend(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        try {
            BlobDescriptor bd = am.statBlob(domain, volume, blobName);
            long byteCount = FdsFileSystem.getByteCount(bd);
            ObjectOffset objectOffset = getObjectOffset(byteCount, objectSize);
            int length = (int) byteCount % objectSize;
            ByteBuffer lastObject = am.getBlob(domain, volume, blobName, length, objectOffset);
            ByteBuffer currentBuffer = ByteBuffer.allocate(objectSize);
            currentBuffer.put(lastObject);
            return new FdsOutputStream(objectSize, domain, volume, blobName, am, byteCount, currentBuffer, false, false);
        } catch (TException e) {
            throw new IOException(e);
        }
    }


    public static FdsOutputStream openNew(AmService.Iface am, String domain, String volume, String blobName, int objectSize) throws IOException {
        try {
            am.updateBlobOnce(domain, volume, blobName, 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), makeMetadata(0));
            return new FdsOutputStream(am, domain, volume, blobName, objectSize);
        } catch (TException e) {
            throw new IOException(e);
        }
    }

    private static Map<String, String> makeMetadata(long currentOffset) {
        HashMap<String, String> md = new HashMap<>();
        md.put(FdsFileSystem.LAST_MODIFIED_KEY, Long.toString(Calendar.getInstance().getTimeInMillis()));
        md.put(FdsFileSystem.CURRENT_OFFSET, Long.toString(currentOffset));
        return md;
    }


    @Override
    public void flush() throws IOException {
        if (isClosed) {
            throw new IOException("Stream was closed");
        }

        if (isDirty) {
            ObjectOffset objectOffset = getObjectOffset(currentOffset - 1, objectSize);
            currentBuffer.flip();

            try {
                am.updateBlobOnce(domain, volume, blobName, 1, currentBuffer, currentBuffer.limit(), objectOffset, makeMetadata(currentOffset));
            } catch (TException e) {
                isClosed = true;
                throw new IOException(e);
            }

            isDirty = false;
            currentBuffer.position(0);
            currentBuffer.limit(objectSize);
        }
    }

    private static ObjectOffset getObjectOffset(long atOffset, int objectSize) {
        return new ObjectOffset((long) Math.floor(((double)atOffset) / ((double) objectSize)));
    }

    @Override
    public void write(byte[] b, int off, int len) throws IOException {
        for (int i = off; i < len; i++) {
            write(b[i]);
        }
    }

    @Override
    public void write(int b) throws IOException {
        if (! (currentBuffer.position() < objectSize)) {
            flush();
        }
        currentBuffer.put((byte) b);
        isDirty = true;
        currentOffset++;
        isDirty = true;
    }

    @Override
    public void write(byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    @Override
    public void close() throws IOException {
        if (isClosed) {
            return;
        }
        flush();
        super.close();
        isClosed = true;
    }
}
