package com.formationds.util;

import java.io.IOException;

public interface IoFunction<TIn, TOut> {
    public TOut apply(TIn in) throws IOException;
}
