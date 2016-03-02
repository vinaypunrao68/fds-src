package com.formationds.index;

import com.formationds.util.IoSupplier;
import org.apache.lucene.store.Lock;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

public class MemoryLock extends Lock {
    private final AtomicBoolean isValid;
    private IoSupplier<Lock> onClose;

    public MemoryLock(IoSupplier<Lock> onClose) {
        this.onClose = onClose;
        isValid = new AtomicBoolean(true);
    }

    public void invalidate() {
        isValid.set(false);
    }
    @Override
    public void close() throws IOException {
        if (!isValid.get()) {
            throw new IOException("Already closed or invalidated");
        }
        invalidate();
        onClose.supply();
    }

    @Override
    public void ensureValid() throws IOException {
        if (!isValid.get()) {
            throw new IOException("Invalid lock");
        }
    }
}
