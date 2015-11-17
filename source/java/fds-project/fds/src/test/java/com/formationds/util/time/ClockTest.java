package com.formationds.util.time;

import org.junit.Test;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import static org.junit.Assert.*;

public class ClockTest {
    @Test
    public void testUnitTestClockDelay() throws InterruptedException, ExecutionException, TimeoutException {
        UnitTestClock unitTestClock = new UnitTestClock(1000);

        CompletableFuture<Void> shortDelay = unitTestClock.delay(100, TimeUnit.MILLISECONDS);
        CompletableFuture<Void> longDelay = unitTestClock.delay(100, TimeUnit.SECONDS);

        unitTestClock.setMillisAsync(unitTestClock.currentTimeMillis() + 1000);

        shortDelay.get(1, TimeUnit.SECONDS);
        assertTrue(!longDelay.isDone());
    }

    @Test
    public void testSystemClockDelay() throws InterruptedException, TimeoutException, ExecutionException {
        SystemClock systemClock = SystemClock.current();

        CompletableFuture<Void> shortDelay = systemClock.delay(1, TimeUnit.MILLISECONDS);
        CompletableFuture<Void> longDelay = systemClock.delay(100, TimeUnit.SECONDS);

        shortDelay.get(1, TimeUnit.SECONDS);
        assertTrue(!longDelay.isDone());
    }
}