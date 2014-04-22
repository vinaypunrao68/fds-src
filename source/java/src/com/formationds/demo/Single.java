package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.concurrent.CountDownLatch;

public class Single<T> {
    private CountDownLatch latch;
    private T tee;

    public Single() {
        latch = new CountDownLatch(1);
    }

    public void set(T t) {
        this.tee = t;
        latch.countDown();
    }

    public T get() {
        try {
            latch.await();
        } catch (InterruptedException e) {

        }
        return tee;
    }
}
