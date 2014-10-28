package com.formationds.util;

public interface BiConsumerWithException<A, B> {
    public void accept(A a, B b) throws Exception;
}
