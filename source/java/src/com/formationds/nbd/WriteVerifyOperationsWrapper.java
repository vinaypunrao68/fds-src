package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;

import java.util.ArrayList;
import java.util.concurrent.CompletableFuture;

public class WriteVerifyOperationsWrapper implements NbdServerOperations {
    private NbdServerOperations inner;

    public WriteVerifyOperationsWrapper(NbdServerOperations inner) {
        this.inner = inner;
    }

    @Override
    public boolean exists(String exportName) {
        return inner.exists(exportName);
    }

    @Override
    public long size(String exportName) {
        return inner.size(exportName);
    }

    @Override
    public CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        return inner.read(exportName, target, offset, len);
    }

    @Override
    public CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        inner.write(exportName, source, offset, len);

        ByteBuf v_buf = Unpooled.buffer(len);
        CompletableFuture<Void> cv = read(exportName, v_buf, offset, len);


        return cv.thenCompose(v -> {
                    CompletableFuture<Void> result = new CompletableFuture<>();
                    try {
                        if (v_buf.compareTo(source) != 0) {
                            ArrayList<Integer> differences = new ArrayList<>();
                            for (int i = 0; i < len; i++) {
                                if (v_buf.getByte(i) != source.getByte(i))
                                    differences.add(i);
                            }
                            throw new Exception("Write verification failed");
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
    public CompletableFuture<Void> flush(String exportName) {
        return inner.flush(exportName);
    }
}
