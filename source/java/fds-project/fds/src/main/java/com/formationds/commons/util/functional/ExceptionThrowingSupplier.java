package com.formationds.commons.util.functional;

@FunctionalInterface
public interface ExceptionThrowingSupplier<R, E extends Exception>
{
    R get() throws E;
}
