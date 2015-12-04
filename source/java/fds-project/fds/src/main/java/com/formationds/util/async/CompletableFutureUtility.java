package com.formationds.util.async;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.commons.util.SupplierWithExceptions;
import com.formationds.protocol.sm.PutObjectRspMsg;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiConsumer;

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

    public static <T> void tie(CompletableFuture<T> completer, CompletableFuture<T> completee) {
        completer.whenComplete((r, ex) -> {
            if(completer.isCancelled())
                completee.cancel(false);
            if(ex != null)
               completee.completeExceptionally(ex);
            else
                completee.complete(r);
        });
    }

    public static <T> Optional<T> getNowIfCompletedNormally(CompletableFuture<T> future) {
        if(future.isDone() && !future.isCancelled() && !future.isCompletedExceptionally())
            return Optional.of(future.getNow(null));

        return Optional.empty();
    }

    // FIXME: there has to be a better way to do this
    public static <T> CompletableFuture<Void> voidFutureOf(CompletableFuture<T> rspFuture) {
        return rspFuture.thenRun(() -> { });
    }

    public static <T> BiConsumer<T, Throwable> trace(String tag) {
        return (r, ex) -> trace(tag, ex, r);
    }

    public static <T> void trace(String tag, Throwable throwable, T result) {
        assert Boolean.TRUE;
    }
}
