package com.formationds.util.async;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.async.CompletableFutureUtility;

import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.CompletableFuture;

public class AsyncResourcePool<TPoolObject> {
    private Queue<TPoolObject> objects;
    private Queue<CompletableFuture<TPoolObject>> requests;
    private int objectLimit;
    private int inUse;
    private final Object lock = new Object();
    private Impl<TPoolObject> impl;

    public int getUsedCount() {
        return inUse;
    }

    public interface BindWithException<T,R> {
        public R apply(T input) throws Exception;
    }

    public interface Impl<T> {
        public T construct() throws Exception;
        public void destroy(T elt);
        public boolean isValid(T elt);
    }

    public AsyncResourcePool(Impl<TPoolObject> impl, int objectLimit) {
        this.impl = impl;
        this.objectLimit = objectLimit;
        this.objects = new LinkedList<>();
        this.requests = new LinkedList<>();
        inUse = 0;
    }

    public <TResult> CompletableFuture<TResult> use(BindWithException<TPoolObject, CompletableFuture<TResult>> f) {
        CompletableFuture<TPoolObject> request = null;
        synchronized (lock) {
            // grab an object from the pool
            while(!objects.isEmpty()) {
                TPoolObject obj = objects.remove();
                if(!impl.isValid(obj)) {
                    impl.destroy(obj);
                    continue;
                }

                request = CompletableFuture.completedFuture(obj);
                inUse++;
                break;
            }

            if(request == null) {
                if (inUse >= objectLimit) {
                    // if the pool is empty generate a request
                    request = new CompletableFuture<>();
                    requests.add(request);
                } else {
                    // otherwise, create an object (done outside of synchronized block)
                    inUse++;
                }
            }
        }

        // create a new pool object
        if(request == null) {
            try {
                request = CompletableFuture.completedFuture(impl.construct());
            } catch(Exception e) {
                // compensate for construction failure
                synchronized (lock) {
                    inUse--;
                }
                return CompletableFutureUtility.exceptionFuture(e);
            }
        }

        final CompletableFuture<TPoolObject> finalRequest = request;
        return request.thenCompose(r -> bind(f, r)).whenComplete((r, ex) -> {
            try { returnObject(finalRequest.get(), ex); }
            catch(Exception e) { returnObject(null, ex);  }
        });
    }

    private <T,R> CompletableFuture<R> bind(BindWithException<T, CompletableFuture<R>> bindingFunction, T input) {
        try {
            return bindingFunction.apply(input);
        } catch(Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }


    private void returnObject(TPoolObject obj, Throwable ex) {
        CompletableFuture<TPoolObject> request = null;
        synchronized (lock) {
            if(requests.size() > 0) {
                request = requests.remove();
            } else {
                inUse--;
                if(obj != null && impl.isValid(obj))
                    objects.add(obj);
                return;
            }
        }

        if(obj == null || !impl.isValid(obj)) {
            try {
                if(obj != null)
                    impl.destroy(obj);
                obj = impl.construct();
            } catch(Exception e) {
                if(request != null)
                    request.completeExceptionally(e);
            }
        }

        if(request != null) {
            request.complete(obj);
        }
    }
}
