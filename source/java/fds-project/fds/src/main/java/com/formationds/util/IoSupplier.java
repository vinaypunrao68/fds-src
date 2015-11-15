package com.formationds.util;

import java.io.IOException;

public interface IoSupplier<T> {
    public T supply() throws IOException;
}
