package com.formationds.util.async;

import com.formationds.util.FunctionWithExceptions;

import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;
import java.util.function.Function;

public class AsyncFlow {

    public static CompletableFuture<Void> loop(FunctionWithExceptions<AsyncLoopHandle<Void>, CompletableFuture<Void>> loopBody) {
        AsyncLoopHandle<Void> handle = new AsyncLoopHandle<>();
        return loopOperation(handle, loopBody);
    }

    private static CompletableFuture<Void> loopOperation(AsyncLoopHandle<Void> handle, FunctionWithExceptions<AsyncLoopHandle<Void>, CompletableFuture<Void>> f) {
        try {
            return f.apply(handle).thenCompose(_null -> {
                if(handle.yieldSet)
                    return CompletableFuture.completedFuture(null);
                return loopOperation(handle, f);
            });
        } catch(Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    public static class AsyncLoopHandle<T> {
        private boolean yieldSet;
        private T yieldValue;

        public CompletableFuture<T> complete(T result) {
            yieldSet = true;
            yieldValue = result;
            return CompletableFuture.completedFuture(result);
        }
    }
}
