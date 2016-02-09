package com.formationds.util;

import java.io.IOException;

public interface IoConsumer<T> {
    public void accept(T tee) throws IOException;
}
