package com.formationds.util;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
public interface FunctionWithExceptions<TIn, TOut> {
    public TOut apply(TIn input) throws Exception;
}

