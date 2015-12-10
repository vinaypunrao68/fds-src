package com.formationds.util.executor;

import com.formationds.util.Lazy;

import java.util.concurrent.*;

// an unbounded executor intended to allow process-wide thread reuse
public class ProcessExecutorSource implements Executor {
    private static Lazy<ProcessExecutorSource> current = new Lazy<>(ProcessExecutorSource::new);
    private Executor executor;

    private ProcessExecutorSource() {
        executor = Executors.newCachedThreadPool();
    }

    public static ProcessExecutorSource getInstance() {
        return current.getInstance();
    }

    @Override
    public void execute(Runnable command) {
        executor.execute(command);
    }

    // get an executor that is bounded, but uses threads from this unbounded executor
    public Executor boundedSubExecutor(String threadBaseName, int concurrentThreads) {
        return new SubExecutor(this, concurrentThreads, threadBaseName);
    }

    public <T> void completeAsync(CompletableFuture<T> completionHandle, T result) {
        execute(() -> completionHandle.complete(result));
    }
}
