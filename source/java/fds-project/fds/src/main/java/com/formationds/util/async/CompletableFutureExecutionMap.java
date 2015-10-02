package com.formationds.util.async;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

public class CompletableFutureExecutionMap<TKey> {

    private Map<TKey, CompletableFuture<Void>> cfMap;

    public CompletableFutureExecutionMap() {
        cfMap = new HashMap<>();
    }

    public <T> CompletableFuture<T> schedule(TKey key, Supplier<CompletableFuture<T>> value) {
        synchronized (cfMap) {
            CompletableFuture<T> result;

            if(cfMap.containsKey(key)) {
                result = cfMap.get(key).thenCompose(c -> value.get());
            } else {
                result = value.get();
            }

            CompletableFuture<Void> v = result.thenRun(() -> {  });
            cfMap.put(key, v);
            v.thenRun(() -> cleanup(key, v));
            return result;
        }
    }

    private void cleanup(TKey key, CompletableFuture<Void> f) {
        synchronized (cfMap) {
            if(cfMap.containsKey(key) && cfMap.get(key) == f)
                cfMap.remove(key);
        }
    }
}
