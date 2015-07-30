package com.formationds.hadoop;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ObjectOffset;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.xdi.AsyncAm;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Calendar;
import java.util.HashMap;
import java.util.Map;

import static com.formationds.hadoop.FdsFileSystem.unwindExceptions;

public class FdsOutputStream extends OutputStream {
    private final int objectSize;
    private final String domain;
    private final String volume;
    private final String blobName;
    private AsyncAm asyncAm;
    private long currentOffset;
    private ByteBuffer currentBuffer;
    private boolean isDirty;
    private boolean isClosed;
    private OwnerGroupInfo ownerGroupInfo;

    FdsOutputStream(AsyncAm am, String domain, String volume, String blobName, int objectSize, OwnerGroupInfo ownerGroupInfo) throws IOException {
        this(objectSize, domain, volume, blobName, am, 0, ByteBuffer.allocate(objectSize), false, false, ownerGroupInfo);
    }

    private FdsOutputStream(int objectSize, String domain, String volume, String blobName, AsyncAm asyncAm,
                            long currentOffset, ByteBuffer currentBuffer, boolean isDirty, boolean isClosed, OwnerGroupInfo ownerGroupInfo) {
        this.objectSize = objectSize;
        this.domain = domain;
        this.volume = volume;
        this.blobName = blobName;
        this.asyncAm = asyncAm;
        this.currentOffset = currentOffset;
        this.currentBuffer = currentBuffer;
        this.isDirty = isDirty;
        this.isClosed = isClosed;
        this.ownerGroupInfo = ownerGroupInfo;
    }

    public static FdsOutputStream openForAppend(AsyncAm asyncAm, String domain, String volume, String blobName, int objectSize) throws IOException {
        try {
            BlobDescriptor bd = asyncAm.statBlob(domain, volume, blobName).get();
            OwnerGroupInfo owner = new OwnerGroupInfo(bd);
            long byteCount = bd.getByteCount();
            ObjectOffset objectOffset = getObjectOffset(byteCount, objectSize);
            int length = (int) byteCount % objectSize;
            ByteBuffer lastObject = asyncAm.getBlob(domain, volume, blobName, length, objectOffset).get();
            ByteBuffer currentBuffer = ByteBuffer.allocate(objectSize);
            currentBuffer.put(lastObject);
            return new FdsOutputStream(objectSize, domain, volume, blobName, asyncAm, byteCount, currentBuffer, false, false, owner);
        } catch (Exception e) {
            throw new IOException(e);
        }
    }


    public static FdsOutputStream openNew(AsyncAm asyncAm, String domain, String volume, String blobName, int objectSize, OwnerGroupInfo owner) throws IOException {
        try {
            unwindExceptions(() -> asyncAm.updateBlobOnce(domain, volume, blobName, 1, ByteBuffer.allocate(0), 0, new ObjectOffset(0), makeMetadata(owner)).get());
            return new FdsOutputStream(asyncAm, domain, volume, blobName, objectSize, owner);
        } catch (Exception e) {
            throw new IOException(e);
        }
    }

    private static Map<String, String> makeMetadata(OwnerGroupInfo ownerGroupInfo) {
        HashMap<String, String> md = new HashMap<>();
        md.put(FdsFileSystem.LAST_MODIFIED_KEY, Long.toString(Calendar.getInstance().getTimeInMillis()));
        md.put(FdsFileSystem.CREATED_BY_USER, ownerGroupInfo.getOwner());
        md.put(FdsFileSystem.CREATED_BY_GROUP, ownerGroupInfo.getGroup());
        return md;
    }


    @Override
    public void flush() throws IOException {
        if (isClosed) {
            return;
        }

        if (isDirty) {
            ObjectOffset objectOffset = getObjectOffset(currentOffset - 1, objectSize);
            currentBuffer.flip();

            try {
                long currentLength = length();
                long lastObject = Math.floorDiv(currentLength, objectSize);
                int[] truncate = new int[]{1};
                if (lastObject < objectOffset.getValue() && currentBuffer.remaining() == objectSize) {
                    truncate[0] = 0;
                }
                unwindExceptions(() -> asyncAm.updateBlobOnce(domain, volume, blobName, truncate[0], currentBuffer, currentBuffer.limit(), objectOffset, makeMetadata(ownerGroupInfo)).get());
            } catch (Exception e) {
                isClosed = true;
                throw new IOException();
            }

            isDirty = false;
            currentBuffer.position((int) (currentOffset % objectSize));
            currentBuffer.limit(objectSize);
        }
    }

    public long length() throws Exception {
        return unwindExceptions(() -> asyncAm.statBlob(domain, volume, blobName).get()).getByteCount();
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
        if (!(currentBuffer.position() < objectSize)) {
            flush();
        }
        currentBuffer.put((byte) b);
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
        try {
            try {
                flush();
            } finally {
                super.close();
            }
        }finally {
            isClosed = true;
        }
    }
}
