package com.formationds.nbd;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

import java.util.*;
import java.util.concurrent.CompletableFuture;

public class BlockExclusionWrapper implements NbdServerOperations {

    private NbdServerOperations inner;
    private int blockSize;
    private Map<Long, CompletableFuture<Void>> pendingOperationMap;

    public BlockExclusionWrapper(NbdServerOperations inner, int blockSize) {
        this.inner = inner;
        this.blockSize = blockSize;
        this.pendingOperationMap = new HashMap<>();
    }

    @Override
    public boolean exists(String exportName) {
        return inner.exists(exportName);
    }

    @Override
    public long size(String exportName) {
        return inner.size(exportName);
    }

    public long rangeMin(long offset) {
        return offset / blockSize;
    }

    public long rangeMax(long offset, long length) {
        return (offset + Math.max(length - 1, 0)) / blockSize;
    }

    private void cleanMap() {
        pendingOperationMap.entrySet().removeIf(e -> e.getValue().isDone());
    }

    private CompletableFuture<Void> orderOperation(long offset, int len) {
        cleanMap();  // optimize: targeted cleanup
        IdentityHashMap<CompletableFuture<Void>, Void> outstandingOperations = new IdentityHashMap<>();
        for(long i = rangeMin(offset); i <= rangeMax(offset, len); i++) {
            if(pendingOperationMap.containsKey(i)) {
                CompletableFuture<Void> operation = pendingOperationMap.get(i);
                if(!outstandingOperations.containsKey(operation)) {
                    outstandingOperations.put(operation, null);
                }
            }

        }
        Set<CompletableFuture<Void>> pendingOps = outstandingOperations.keySet();
        if(!pendingOps.isEmpty()) {
            CompletableFuture[] pendingOpsArray = pendingOps.toArray(new CompletableFuture[pendingOps.size()]);
            return CompletableFuture.allOf(pendingOpsArray);
        } else {
            return CompletableFuture.completedFuture(null);
        }
    }

    private void setOperation(long offset, int len, CompletableFuture<Void> op) {
        for(long i = rangeMin(offset); i <= rangeMax(offset, len); i++) {
            pendingOperationMap.put(i, op);
        }
    }

    @Override
    public CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        synchronized (pendingOperationMap) {
            CompletableFuture<Void> orderedResult = orderOperation(offset, len).thenCompose(i -> {
                CompletableFuture<Void> result;
                try {
                    result = inner.read(exportName, target, offset, len);
                } catch (Exception e) {
                    CompletableFuture<Void> c = new CompletableFuture<>();
                    c.completeExceptionally(e);
                    result = c;
                }
                return result;
            });

            setOperation(offset, len, orderedResult);
            return orderedResult;
        }
    }

    @Override
    public CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        synchronized (pendingOperationMap) {
            CompletableFuture<Void> orderedResult = orderOperation(offset, len).thenCompose(i -> {
                CompletableFuture<Void> result;
                try {
                    result = inner.write(exportName, source, offset, len);
                } catch (Exception e) {
                    CompletableFuture<Void> c = new CompletableFuture<>();
                    c.completeExceptionally(e);
                    result = c;
                }
                return result;
            });

            setOperation(offset, len, orderedResult);
            return orderedResult;
        }
    }

    @Override
    public CompletableFuture<Void> flush(String exportName) {
        return CompletableFuture.completedFuture(null);
    }
}
