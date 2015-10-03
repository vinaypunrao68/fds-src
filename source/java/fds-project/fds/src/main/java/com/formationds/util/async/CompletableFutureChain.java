package com.formationds.util.async;

import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class CompletableFutureChain<T> {

    private CompletableFuture<T> last;
    private Object lock;

    public CompletableFutureChain(T inital) {
        last = CompletableFuture.completedFuture(inital);
        lock = new Object();
    }

    public CompletableFuture<T> schedule(Function<T, CompletableFuture<T>> next) {
        synchronized (lock) {
            last = last.thenCompose(next);
            return last;
        }
    }
}
