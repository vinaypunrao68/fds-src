package com.formationds.nfs;

import com.formationds.util.IoSupplier;

import java.io.IOException;
import java.util.concurrent.locks.ReentrantLock;

class StripedLock {
    private ReentrantLock[] locks;
    private static int LOCK_STRIPES = 30000;

    public StripedLock() {
        locks = new ReentrantLock[LOCK_STRIPES];
        for (int i = 0; i < locks.length; i++) {
            locks[i] = new ReentrantLock();
        }
    }

    public <T> T lock(Object object, IoSupplier<T> supplier) throws IOException {
        int stripe = Math.abs(object.hashCode() % LOCK_STRIPES);
        locks[stripe].lock();
        try {
            return supplier.supply();
        } finally {
            locks[stripe].unlock();
        }
    }
}
