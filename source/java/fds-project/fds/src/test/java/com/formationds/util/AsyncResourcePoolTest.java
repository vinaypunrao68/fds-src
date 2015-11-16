package com.formationds.util;

import com.formationds.util.async.AsyncResourcePool;
import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public class AsyncResourcePoolTest {
    private class Resource {
        public long id;
        public boolean constructed;
        public boolean isValid;
        public boolean destroyed;

        public int square(int i) {
            return i * i;
        }
    }

    private class InvalidateResourceException extends Exception {

    }

    private class ResourceImpl implements AsyncResourcePool.Impl<Resource> {
        public long currentId = 0;
        public final Object lock = new Object();
        public ArrayList<Resource> allResources = new ArrayList<>();

        @Override
        public Resource construct() throws Exception {
            Resource r = new Resource();
            synchronized (lock) {
                r.constructed = true;
                r.id = currentId++;
                r.isValid = true;
                allResources.add(r);
            }
            return r;
        }

        @Override
        public void destroy(Resource elt) {
            elt.destroyed = true;
        }

        @Override
        public boolean isValid(Resource elt) {
            return elt.isValid;
        }
    }

    private AsyncResourcePool<Resource> makePool(int size) {
        return new AsyncResourcePool<>(new ResourceImpl(), size);
    }

    @Test
    public void testBasicPooling() throws Exception {
        ResourceImpl impl = new ResourceImpl();
        AsyncResourcePool<Resource> pool = new AsyncResourcePool<>(impl, 10);

        HashMap<Integer, CompletableFuture<Integer>> results = new HashMap<>();
        CompletableFuture<Void> barrier = new CompletableFuture<>();
        for (int i = 0; i < 1000; i++) {
            final int clsi = i;
            CompletableFuture<Integer> result = pool.use(resource ->
                    barrier.thenComposeAsync(_null ->
                        CompletableFuture.supplyAsync(() -> resource.square(clsi))));

            results.put(i, result);
        }

        barrier.complete(null);

        for(Map.Entry<Integer,CompletableFuture<Integer>> result : results.entrySet()) {
            int square = result.getKey() * result.getKey();
            Assert.assertEquals(square, result.getValue().get(10000, TimeUnit.MILLISECONDS).intValue());
        }

        Assert.assertEquals(10, impl.allResources.stream().filter(r -> !r.destroyed).count());
    }

    @Test
    public void testReuse() throws Exception {
        ResourceImpl impl = new ResourceImpl();
        AsyncResourcePool<Resource> pool = new AsyncResourcePool<>(impl, 10);

        Resource r1 = pool.use(r -> CompletableFuture.completedFuture(r)).get();
        Resource r2 = pool.use(r -> CompletableFuture.completedFuture(r)).get();

        Assert.assertEquals(r1, r2);
    }

    @Test
    public void testInvalidate() throws Exception {
        ResourceImpl impl = new ResourceImpl();
        AsyncResourcePool<Resource> pool = new AsyncResourcePool<>(impl, 10);

        Resource r1 = pool.use(r -> CompletableFuture.completedFuture(r)).get();
        r1.isValid = false;
        Resource r2 = pool.use(r -> CompletableFuture.completedFuture(r)).get();

        Assert.assertNotEquals(r1, r2);
    }
}