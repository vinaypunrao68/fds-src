package com.formationds.index;

import org.apache.lucene.store.Lock;

import java.io.IOException;

public class MemoryLock extends Lock {
    private boolean isLocked = false;

    @Override
    public synchronized boolean obtain() throws IOException {
        if (!isLocked) {
            isLocked = true;
            return isLocked;
        }

        return false;
    }

    @Override
    public synchronized void close() throws IOException {
        isLocked = false;
    }

    @Override
    public synchronized boolean isLocked() throws IOException {
        return isLocked;
    }
}
