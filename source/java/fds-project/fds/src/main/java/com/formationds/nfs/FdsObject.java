package com.formationds.nfs;

import com.formationds.util.IoFunction;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.locks.ReentrantLock;

public class FdsObject {
    private final ReentrantLock lock;
    private final int maxObjectSize;
    private int limit;
    private byte[] bytes;

    public FdsObject(ByteBuffer byteBuffer, int maxObjectSize) {
        lock = new ReentrantLock();
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

    public <T> T lock(IoFunction<Accessor, T> f) throws IOException {
        lock.lock();
        try {
            return f.apply(new Accessor());
        } finally {
            lock.unlock();
        }
    }

    /**
     * Not thread-safe, use lock()
     */
    public int limit() {
        return limit;
    }

    /**
     * Not thread-safe, use lock()
     */
    public int maxObjectSize() {
        return maxObjectSize;
    }

    public class Accessor {
        public int limit() {
            return limit;
        }

        public void limit(int value) {
            FdsObject.this.limit = value;
        }

        public int maxObjectSize() {
            return maxObjectSize;
        }

        public ByteBuffer asByteBuffer() {
            ByteBuffer bb = ByteBuffer.wrap(bytes);
            bb.limit(limit);
            return bb;
        }

        public byte[] toByteArray() {
            byte[] byteArray = new byte[limit];
            System.arraycopy(bytes, 0, byteArray, 0, limit);
            return byteArray;
        }

        public FdsObject fdsObject() {
            return FdsObject.this;

        }
    }
}
