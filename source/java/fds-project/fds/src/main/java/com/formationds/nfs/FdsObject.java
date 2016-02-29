package com.formationds.nfs;

import com.formationds.util.IoFunction;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.locks.ReentrantLock;

public class FdsObject {
    private final int maxObjectSize;
    private int limit;
    private byte[] bytes;

    public FdsObject(ByteBuffer byteBuffer, int maxObjectSize) {
        bytes = new byte[maxObjectSize];
        this.maxObjectSize = maxObjectSize;
        this.limit = byteBuffer.remaining();
        ByteBuffer.wrap(bytes).put(byteBuffer);
    }

    public static FdsObject wrap(byte[] bytes, int maxObjectSize) {
        return new FdsObject(ByteBuffer.wrap(bytes), maxObjectSize);
    }

    public static FdsObject allocate(int size, int maxObjectSize) {
        return wrap(new byte[size], maxObjectSize);
    }

    public int limit() {
        return limit;
    }

    public int capacity() {
        return bytes.length;
    }

    public void limit(int limit) {
        this.limit = limit;
    }

    /**
     * Not thread-safe, use lock()
     */
    public int maxObjectSize() {
        return maxObjectSize;
    }

    public ByteBuffer asByteBuffer() {
        ByteBuffer byteBuffer = ByteBuffer.wrap(bytes);
        byteBuffer.limit(limit);
        return byteBuffer;
    }

    public byte[] toByteArray() {
        return bytes;
    }
}
