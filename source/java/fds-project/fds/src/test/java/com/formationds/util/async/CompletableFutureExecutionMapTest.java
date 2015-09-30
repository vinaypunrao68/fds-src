package com.formationds.util.async;

import org.junit.Test;

import java.util.concurrent.CompletableFuture;

import static org.junit.Assert.*;

public class CompletableFutureExecutionMapTest {
    @Test
    public void testBasic() {
        CompletableFutureExecutionMap<Integer> exmap = new CompletableFutureExecutionMap<>();

        CompletableFuture<Void> cf1a = new CompletableFuture<>();
        CompletableFuture<Void> cf1b = new CompletableFuture<>();
        CompletableFuture<Void> cf1c = new CompletableFuture<>();
        CompletableFuture<Void> cf2 = new CompletableFuture<>();

        exmap.schedule(1, () -> cf1a);
        exmap.schedule(1, () -> { cf1b.complete(null); return cf1b; });
        exmap.schedule(1, () -> { cf1c.complete(null); return cf1c; });
        exmap.schedule(2, () -> { cf2.complete(null); return cf2; } );

        assertFalse(cf1b.isDone());
        assertFalse(cf1c.isDone());
        assertTrue(cf2.isDone());

        cf1a.complete(null);

        assertTrue(cf1b.isDone());
        assertTrue(cf1c.isDone());
    }
}