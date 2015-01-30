package com.formationds.commons.util.functional;

@FunctionalInterface
public interface ExceptionThrowingRunnable<E extends Exception>
{
    void run() throws E;
}
