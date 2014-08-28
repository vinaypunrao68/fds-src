package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;

public class OracleVerifyOperationsWrapper implements NbdServerOperations {
    private NbdServerOperations innerOperations;
    private NbdServerOperations oracle;

    public OracleVerifyOperationsWrapper(NbdServerOperations inner, NbdServerOperations oracle) {

        innerOperations = inner;
        this.oracle = oracle;
    }

    @Override
    public boolean exists(String exportName) {
        return innerOperations.exists(exportName);
    }

    @Override
    public long size(String exportName) {
        return innerOperations.size(exportName);
    }

    @Override
    public CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        ByteBuf ramTarget = target.copy();
        CompletableFuture<Void> oracle_read = oracle.read(exportName, ramTarget, offset, len);
        CompletableFuture<Void> inner_read = innerOperations.read(exportName, target, offset, len);

        return CompletableFuture.allOf(oracle_read, inner_read)
                .thenCompose(v -> {
                    CompletableFuture<Void> result = new CompletableFuture<>();
                    try {
                        ArrayList<Integer> differences = new ArrayList<>();
                        if (target.compareTo(ramTarget) != 0) {
                            for (int i = 0; i < len; i++) {
                                if (target.getByte(i) != ramTarget.getByte(i))
                                    differences.add(i);
                            }

                            if (differences.size() > 0)
                                throw new Exception("read buffer does not match oracle");
                        }
                    }
                    catch(Exception e) {
                        result.completeExceptionally(e);
                        return result;
                    }
                    result.complete(null);
                    return result;
                });
    }

    @Override
    public CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        ByteBuf ramSource = source.copy();
        CompletableFuture<Void> oracle_write = oracle.write(exportName, ramSource, offset, len);
        CompletableFuture<Void> inner_write = innerOperations.write(exportName, source, offset, len);

        return CompletableFuture.allOf(oracle_write, inner_write);
    }

    @Override
    public CompletableFuture<Void> flush(String exportName) {
        return CompletableFuture.allOf(innerOperations.flush(exportName), oracle.flush(exportName));
    }
}
