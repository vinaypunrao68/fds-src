package com.formationds.util;

public interface ConsumerWithException<A> {
    public void accept(A a) throws Exception;
}
