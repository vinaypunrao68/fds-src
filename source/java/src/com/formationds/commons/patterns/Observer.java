package com.formationds.commons.patterns;

@FunctionalInterface
public interface Observer<T>
{
    void observe(T value);
}
