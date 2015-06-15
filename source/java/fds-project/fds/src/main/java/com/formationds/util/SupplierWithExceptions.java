package com.formationds.util;

public interface SupplierWithExceptions<T> {
    public T supply() throws Exception;
}
