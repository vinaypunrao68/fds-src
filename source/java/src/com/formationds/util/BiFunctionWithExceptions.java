package com.formationds.util;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public interface BiFunctionWithExceptions<TIn1, Tin2, TOut> {
    public TOut apply(TIn1 one, Tin2 two) throws Exception;
}

