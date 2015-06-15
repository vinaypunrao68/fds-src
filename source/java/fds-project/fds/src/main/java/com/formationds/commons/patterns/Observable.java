package com.formationds.commons.patterns;

import java.io.Closeable;

public interface Observable<T>
{
    Closeable register(Observer<? super T> observer);
    
    boolean unregister(byte[] registrationKey);
}
