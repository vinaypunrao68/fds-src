package com.formationds.util.time;

import com.formationds.util.Lazy;
import com.formationds.util.executor.ProcessExecutorSource;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class SystemClock implements Clock {
    private static Lazy<SystemClock> lazy = new Lazy<>(SystemClock::new);
    private final ScheduledExecutorService scheduler;

    public static SystemClock current() {
        return lazy.getInstance();
    }

    private SystemClock() {
        scheduler = Executors.newScheduledThreadPool(4);
    }

    @Override
    public long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    @Override
    public long nanoTime() {
        return System.nanoTime();
    }

    @Override
    public CompletableFuture<Void> delay(long delay, TimeUnit timeUnits) {
        if(delay == 0)
            return CompletableFuture.completedFuture(null);

        CompletableFuture<Void> completableFuture = new CompletableFuture<>();
        ProcessExecutorSource pexec = ProcessExecutorSource.getInstance();
        Runnable comp = () -> pexec.execute(() -> completableFuture.complete(null));
        scheduler.schedule(comp, delay, timeUnits);
        return completableFuture;
    }
}
