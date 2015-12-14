package com.formationds.sc;

import org.junit.Test;

import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class TableContainerTest {
    @Test
    public void testBasic() throws Exception {
        TableContainer<Integer> tc = new TableContainer<>(0);

        int t1 = tc.use(t -> CompletableFuture.completedFuture(t + 1)).get(10, TimeUnit.SECONDS);
        assertEquals(1, t1);

        int count = 10000;
        ArrayList<CompletableFuture<Integer>> cfInList = new ArrayList<>();
        ArrayList<CompletableFuture<Integer>> outList = new ArrayList<>();
        for(int i = 0; i < count; i++) {
            CompletableFuture<Integer> r = new CompletableFuture<>();
            cfInList.add(r);
            outList.add(tc.use(v -> r));
        }

        for(CompletableFuture<Integer> in : cfInList)
            in.complete(1);

        for(CompletableFuture<Integer> out : outList)
            assertEquals(1, out.get(10, TimeUnit.SECONDS).intValue());
    }

    @Test
    public void testSuspendResume() throws Exception {
        TableContainer<Integer> tc = new TableContainer<>(0);

        int t1 = tc.use(t -> CompletableFuture.completedFuture(t + 1)).get(10, TimeUnit.SECONDS);
        assertEquals(1, t1);

        tc.suspend();
        CompletableFuture<Integer> t2 = tc.use(i -> CompletableFuture.completedFuture(i + 2));
        Thread.sleep(100);
        if(t2.isDone())
            fail();

        tc.resume();

        assertEquals(2, t2.get(10, TimeUnit.SECONDS).intValue());
    }

    @Test
    public void dltStyleUpdate() throws Exception {
        TableContainer<Integer> tc = new TableContainer<>(0);

        CompletableFuture<Integer> use1 = tc.use(i -> CompletableFuture.completedFuture(i));
        assertEquals(0, use1.get(10, TimeUnit.SECONDS).intValue());

        CompletableFuture<Integer> barrier = new CompletableFuture<>();
        CompletableFuture<Integer> use2 = tc.use(i -> barrier.thenCompose(_null -> CompletableFuture.completedFuture(i)));

        CompletableFuture<Void> update = tc.update(2);

        CompletableFuture<Integer> use3 = tc.use(i -> CompletableFuture.completedFuture(i));
        assertEquals(2, use3.get(10, TimeUnit.SECONDS).intValue());

        Thread.sleep(100);
        assertFalse(update.isDone());

        barrier.complete(0);

        Thread.sleep(100);
        assertTrue(update.isDone());
        assertEquals(0, use2.get(10, TimeUnit.SECONDS).intValue());
    }

    @Test
    public void dmtStyleUpdate() throws Exception {
        TableContainer<Integer> tc = new TableContainer<>(0);

        CompletableFuture<Integer> use1 = tc.use(i -> CompletableFuture.completedFuture(i));
        assertEquals(0, use1.get(10, TimeUnit.SECONDS).intValue());

        CompletableFuture<Integer> barrier = new CompletableFuture<>();
        CompletableFuture<Integer> use2 = tc.use(i -> barrier.thenCompose(_null -> CompletableFuture.completedFuture(i)));

        tc.suspend();
        CompletableFuture<Void> update = tc.update(2);
        CompletableFuture<Integer> use3 = tc.use(i -> CompletableFuture.completedFuture(i));
        Thread.sleep(100);

        assertFalse(update.isDone());
        assertFalse(use2.isDone());
        assertFalse(use3.isDone());

        barrier.complete(0);

        Thread.sleep(100);
        assertTrue(update.isDone());
        tc.resume();

        assertEquals(0, use2.get(10, TimeUnit.SECONDS).intValue());
        assertEquals(2, use3.get(10, TimeUnit.SECONDS).intValue());
    }
}