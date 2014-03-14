package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.concurrent.CountDownLatch;

public final class Mutable<T> {
    private T tee;
    private CountDownLatch latch;

    public Mutable() {
        tee = null;
        latch = new CountDownLatch(1);
    }

    public void set(T tee) {
        synchronized (this) {
            this.tee = tee;
            latch.countDown();
        }
    }

    public T awaitValue() throws InterruptedException {
        latch.await();
        synchronized (this) {
            return tee;
        }
    }
}
