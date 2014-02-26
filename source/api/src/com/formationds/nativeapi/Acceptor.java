package com.formationds.nativeapi;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Consumer;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Acceptor<T> implements Future<T>, Consumer<T> {
    private volatile boolean isDone;
    private T tee;

    public Acceptor() {
        isDone = false;
    }

    @Override
    public boolean cancel(boolean mayInterruptIfRunning) {
        return true;
    }

    @Override
    public boolean isCancelled() {
        return false;
    }

    @Override
    public boolean isDone() {
        return isDone;
    }

    @Override
    public T get() throws InterruptedException, ExecutionException {
        try {
            return get(1, TimeUnit.MINUTES);
        } catch (TimeoutException e) {
            throw new ExecutionException(e);
        }
    }

    @Override
    public T get(long timeout, TimeUnit unit) throws InterruptedException, ExecutionException, TimeoutException {
        synchronized (this) {
            if (!isDone) {
                this.wait();
            }

            return tee;
        }
    }

    @Override
    public synchronized void accept(T t) {
        synchronized (this) {
            if (!isDone) {
                tee = t;
                isDone = true;
            }
            notifyAll();
        }
    }

    @Override
    public Consumer<T> andThen(Consumer<? super T> after) {
        return this;
    }
}
