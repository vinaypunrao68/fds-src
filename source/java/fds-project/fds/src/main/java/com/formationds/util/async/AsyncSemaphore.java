package com.formationds.util.async;

import com.formationds.commons.util.SupplierWithExceptions;

import java.util.concurrent.CompletableFuture;

public class AsyncSemaphore {
    private final AsyncResourcePool<Void> pool;

    public AsyncSemaphore(int capacity) {
        pool = new AsyncResourcePool<>(new VoidImpl(), capacity);
    }

    public <T> CompletableFuture<T> execute(SupplierWithExceptions<CompletableFuture<T>> supplier) {
        return pool.use(_void -> supplier.supply());
    }
}
