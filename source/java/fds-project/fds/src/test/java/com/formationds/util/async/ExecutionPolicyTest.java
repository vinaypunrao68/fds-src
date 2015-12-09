package com.formationds.util.async;

import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.util.async.ExecutionPolicy;
import com.formationds.util.time.UnitTestClock;
import org.junit.Test;

import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.function.Supplier;

import static org.junit.Assert.*;

public class ExecutionPolicyTest {
    @Test
    public void testFailover() throws Exception {
        HashMap<Integer, CompletableFuture<Integer>> map = new HashMap<>();

        map.put(1, new CompletableFuture<>());
        map.put(2, new CompletableFuture<>());
        map.put(3, new CompletableFuture<>());

        ArrayList<Integer> keys = new ArrayList<>(map.keySet());
        keys.sort(Integer::compare);

        final HashSet<Integer> seen = new HashSet<>();
        CompletableFuture<Integer> result = ExecutionPolicy.failover(keys, i -> {
            seen.add(i);
            return map.get(i);
        });

        assertFalse(result.isDone());
        assertTrue(seen.contains(1));
        assertTrue(seen.size() == 1);

        map.get(1).completeExceptionally(new Exception());

        assertFalse(result.isDone());
        assertTrue(seen.contains(2));
        assertTrue(seen.size() == 2);

        map.get(2).completeExceptionally(new Exception());

        map.get(3).complete(3);
        assertEquals(3, result.getNow(0).intValue());
    }

    @Test
    public void testFailoverFailure() throws Exception {
        HashMap<Integer, CompletableFuture<Integer>> map = new HashMap<>();

        map.put(1, new CompletableFuture<>());
        map.put(2, new CompletableFuture<>());
        map.put(3, new CompletableFuture<>());

        ArrayList<Integer> keys = new ArrayList<>(map.keySet());
        keys.sort(Integer::compare);

        CompletableFuture<Integer> result = ExecutionPolicy.failover(keys, map::get);

        map.get(1).completeExceptionally(new Exception());
        map.get(2).completeExceptionally(new Exception());
        map.get(3).completeExceptionally(new Exception());

        assertTrue(result.isCompletedExceptionally());
    }

    @Test
    public void testPrimarySecondary() throws Exception {
        HashMap<Integer, CompletableFuture<Void>> primaries = new HashMap<>();
        HashMap<Integer, CompletableFuture<Void>> secondaries = new HashMap<>();

        primaries.put(1, new CompletableFuture<>());
        primaries.put(2, new CompletableFuture<>());

        secondaries.put(3, new CompletableFuture<>());
        secondaries.put(4, new CompletableFuture<>());

        CompletableFuture<Void> execution = ExecutionPolicy.parallelPrimarySecondary(primaries.keySet(), secondaries.keySet(), i -> primaries.containsKey(i) ? primaries.get(i) : secondaries.get(i));

        assertFalse(execution.isDone());

        secondaries.get(3).completeExceptionally(new Exception());
        assertFalse(execution.isDone());

        primaries.get(1).complete(null);
        assertFalse(execution.isDone());

        primaries.get(2).complete(null);
        assertTrue(execution.isDone() && !execution.isCompletedExceptionally() && !execution.isCancelled());
    }

    @Test
    public void testPrimarySecondaryFailure() throws Exception {
        HashMap<Integer, CompletableFuture<Void>> primaries = new HashMap<>();
        HashMap<Integer, CompletableFuture<Void>> secondaries = new HashMap<>();

        primaries.put(1, new CompletableFuture<>());
        primaries.put(2, new CompletableFuture<>());

        secondaries.put(3, new CompletableFuture<>());
        secondaries.put(4, new CompletableFuture<>());

        CompletableFuture<Void> execution = ExecutionPolicy.parallelPrimarySecondary(primaries.keySet(), secondaries.keySet(), i -> primaries.containsKey(i) ? primaries.get(i) : secondaries.get(i));

        primaries.get(1).complete(null);
        primaries.get(2).completeExceptionally(new Exception());
        assertTrue(execution.isCompletedExceptionally());
    }

    @Test
    public void testRetry() throws Exception {
        LinkedList<CompletableFuture<Integer>> lst = new LinkedList<>();
        for(int i = 0; i < 3; i++) {
            lst.add(new CompletableFuture<>());
        }

        LinkedList<CompletableFuture<Integer>> workingCopy = new LinkedList<>(lst);
        CompletableFuture<Integer> result = ExecutionPolicy.fixedRetry(3, () -> workingCopy.poll());

        assertTrue(!result.isDone());

        lst.get(0).completeExceptionally(new Exception());
        assertTrue(!result.isDone());

        lst.get(1).completeExceptionally(new Exception());
        assertTrue(!result.isDone());

        lst.get(2).complete(5);
        assertEquals(5, result.getNow(0).intValue());
    }

    @Test
    public void testRetryFailure() throws Exception {
        LinkedList<CompletableFuture<Integer>> lst = new LinkedList<>();
        for(int i = 0; i < 3; i++) {
            lst.add(new CompletableFuture<>());
        }

        LinkedList<CompletableFuture<Integer>> workingCopy = new LinkedList<>(lst);
        CompletableFuture<Integer> result = ExecutionPolicy.fixedRetry(3, () -> workingCopy.poll());

        lst.get(0).completeExceptionally(new Exception());
        lst.get(1).completeExceptionally(new Exception());
        lst.get(2).completeExceptionally(new Exception());
        assertTrue(result.isCompletedExceptionally());
    }

    @Test
    public void testBoundedRetryAsTimeout() throws Exception {
        UnitTestClock clock = new UnitTestClock(0);
        CompletableFuture<Void> timeout = clock.delay(1000, TimeUnit.MILLISECONDS);

        Supplier<CompletableFuture<Boolean>> shouldRetry = () ->
                clock.delay(50, TimeUnit.MILLISECONDS)
                .thenCompose(_null -> CompletableFuture.completedFuture(!timeout.isDone()));

        CompletableFuture<Integer> successAfter900ms = ExecutionPolicy.boundedRetry(shouldRetry, () -> {
            if (clock.currentTimeMillis() > 900) {
                return CompletableFuture.completedFuture(10);
            } else {
                return CompletableFutureUtility.exceptionFuture(new Exception());
            }
        });

        CompletableFuture<Integer> expectedFailure = ExecutionPolicy.boundedRetry(shouldRetry, () -> {
            return CompletableFutureUtility.exceptionFuture(new Exception());
        });

        assertTrue(!expectedFailure.isDone());
        assertTrue(!successAfter900ms.isDone());

        clock.setMillisSync(500);
        assertTrue(!expectedFailure.isDone());
        assertTrue(!successAfter900ms.isDone());

        clock.setMillisSync(925);
        assertEquals(10, successAfter900ms.get(10, TimeUnit.SECONDS).intValue());
        assertTrue(!expectedFailure.isDone());

        clock.setMillisSync(999);
        assertTrue(!expectedFailure.isDone());

        clock.setMillisSync(1040);
        assertTrue(!expectedFailure.isDone());

        clock.setMillisSync(1200);

        try {
            expectedFailure.get(10, TimeUnit.SECONDS);
            fail("excpecting exception");
        } catch (Exception ex) {
            assertTrue(expectedFailure.isCompletedExceptionally());
        }
    }

    @Test
    public void testBoundedRetryAsTimeoutFailure() throws Exception {

    }
}