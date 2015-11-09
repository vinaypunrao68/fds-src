package com.formationds.util.async;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.commons.util.SupplierWithExceptions;

import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class CompletableFutureUtility {
    public static <T> CompletableFuture<T> exceptionFuture(Throwable ex) {
        CompletableFuture<T> cf = new CompletableFuture<>();
        cf.completeExceptionally(ex);
        return cf;
    }

    public static <T> CompletableFuture<T> catchExceptions(SupplierWithExceptions<CompletableFuture<T>> cf) {
        try {
            return cf.supply();
        } catch(Exception e) {
            return exceptionFuture(e);
        }
    }


}
