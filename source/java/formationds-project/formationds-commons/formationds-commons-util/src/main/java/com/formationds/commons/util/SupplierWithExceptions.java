package com.formationds.commons.util;

public interface SupplierWithExceptions<T> {
    public T supply() throws Exception;
}
