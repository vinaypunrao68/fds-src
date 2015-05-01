package com.formationds.util;

import org.joda.time.Duration;
import org.junit.Test;

import java.util.concurrent.ExecutionException;

import static org.junit.Assert.assertEquals;

public class RetryTest {
    @Test
    public void testRetry() throws Exception {
        Integer result = new Retry<Integer, Integer>((i, attempt) -> {
            if (attempt < 10) {
                throw new RuntimeException();
            }
            return i;
        }, 11, Duration.millis(10), "test").apply(42);
        assertEquals(42, (int) result);
    }

    @Test(expected = ExecutionException.class)
    public void testRetryFailure() throws Exception {
        new Retry<Integer, Integer>((i, attempt) -> {
            throw new RuntimeException();
        }, 10, Duration.millis(10), "test").apply(42);
    }
}