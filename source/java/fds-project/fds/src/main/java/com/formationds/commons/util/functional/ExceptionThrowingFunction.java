package com.formationds.commons.util.functional;

@FunctionalInterface
public interface ExceptionThrowingFunction<T, R, E extends Exception>
{
    R apply(T t) throws E;
}
