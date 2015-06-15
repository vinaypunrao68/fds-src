package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public interface Mutator<T> {
    public void apply(Mutation<T> mutation);
    public T current();
}