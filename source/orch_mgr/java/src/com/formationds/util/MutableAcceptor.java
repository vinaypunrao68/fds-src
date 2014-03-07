package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.AbstractCollection;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

public class MutableAcceptor<T> extends AbstractCollection<T> {
    private Collection<T> collection;
    private boolean isDone;


    public MutableAcceptor() {
        collection = new ArrayList<>();
        isDone = false;
    }

    @Override
    public Iterator<T> iterator() {
        return collection.iterator();
    }

    @Override
    public int size() {
        return collection.size();
    }

    public boolean isDone() {
        return isDone;
    }

    public void accept(T tee) {
        collection.add(tee);
    }

    public void finish() {
        isDone = true;
    }

}
