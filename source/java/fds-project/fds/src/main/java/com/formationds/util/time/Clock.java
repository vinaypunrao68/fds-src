package com.formationds.util.time;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public interface Clock {
    long currentTimeMillis();
    long nanoTime();
    CompletableFuture<Void> delay(long time, TimeUnit timeUnits);
}
