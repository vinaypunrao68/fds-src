package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Iterator;
import java.util.NoSuchElementException;
import java.util.function.Supplier;

public class CircularIterator<T> implements Iterator<T> {
    private Supplier<Iterator<T>> supplier;
    private Iterator<T> current;
    private T cached;
    private boolean isDirty;

    public CircularIterator(Supplier<Iterator<T>> supplier) {
        this.supplier = supplier;
        isDirty = true;
        current = supplier.get();
        refreshIfNeeded();
        if (cached == null) {
            throw new NoSuchElementException();
        }
    }

    private void refreshIfNeeded() {
        if (isDirty) {
            if (! current.hasNext()) {
                current = supplier.get();
                if (! current.hasNext()) {
                    throw new NoSuchElementException();
                }
            }

            cached = current.next();

            isDirty = false;
        }
    }

    @Override
    public boolean hasNext() {
        return true;
    }

    @Override
    public T next() {
        refreshIfNeeded();
        isDirty = true;
        return cached;
    }
}
