package com.formationds.commons.util.functional;

@FunctionalInterface
public interface ExceptionThrowingBiConsumer<T, U, E extends Exception>
{
    void accept(T t, U u) throws E;
}
