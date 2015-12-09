package com.formationds.sc;
import com.formationds.util.executor.ProcessExecutorSource;

import java.util.HashSet;
import java.util.Iterator;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class TableContainer<TTableType> {
    private Object lock = new Object();
    private HashSet<CompletableFuture<?>> outstanding;
    private HashSet<SuspendedUsage<?>> suspended;
    private TTableType current;
    private CompletableFuture<Void> suspensionComplete;

    public TableContainer(TTableType initialValue) {
        current = initialValue;
        outstanding = new HashSet<>();
        suspended = new HashSet<>();
    }

    public <T> CompletableFuture<T> use(Function<TTableType, CompletableFuture<T>> binding) {
        CompletableFuture<Void> completionHandle;
        TTableType val;
        synchronized (lock) {
            if(suspensionComplete != null) {
                return suspensionComplete.thenCompose(_null -> use(binding));
            } else {
                completionHandle = new CompletableFuture<>();
                outstanding.add(completionHandle);
                val = current;
            }
        }

        return binding.apply(val).whenComplete((tbl, ex) -> {
            synchronized (lock) {
                outstanding.remove(completionHandle);
            }
            ProcessExecutorSource.getInstance().completeAsync(completionHandle, null);
        });
    }

    public void suspend() {
        synchronized (lock) {
            suspensionComplete = new CompletableFuture<Void>();
        }
    }

    public void resume() {
        CompletableFuture<Void> completion = this.suspensionComplete;
        synchronized (lock) {
            completion = suspensionComplete;
            suspensionComplete = null;
        }
        completion.complete(null);
    }

    public CompletableFuture<Void> update(TTableType newValue) {
        synchronized (lock) {
            current = newValue;
            CompletableFuture[] completableFutures = outstanding.toArray(new CompletableFuture[0]);
            outstanding = new HashSet<>();
            return CompletableFuture.allOf(completableFutures).handle((t, ex) -> null);
        }
    }

    private class SuspendedUsage<T> {
        private Function<TTableType, CompletableFuture<T>> suspendedFunction;
        private CompletableFuture<T> completionHandle;

        public SuspendedUsage(Function<TTableType, CompletableFuture<T>> suspendedFunction, CompletableFuture<T> completionHandle) {
            this.suspendedFunction = suspendedFunction;
            this.completionHandle = completionHandle;
        }
    }
}
