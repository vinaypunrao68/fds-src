package com.formationds.om;

import java.io.Serializable;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public interface Mutation<T> extends Serializable {
    public T mutate(T tee);
}