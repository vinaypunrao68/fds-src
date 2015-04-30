package com.formationds.commons.util.functional;

@FunctionalInterface
public interface ExceptionThrowingConsumer<T, E extends Exception>
{
    void accept(T t) throws E;
}
