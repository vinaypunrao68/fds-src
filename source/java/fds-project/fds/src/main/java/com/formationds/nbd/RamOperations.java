package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

import java.util.concurrent.CompletableFuture;

public class RamOperations implements NbdServerOperations {
    private String name;
    private byte[] bytes;

    public RamOperations(String exportName, int size) {

        name = exportName;
        bytes = new byte[size];
    }

    @Override
    public boolean exists(String exportName) {
        return exportName.equalsIgnoreCase(name);
    }

    @Override
    public long size(String exportName) {
        return bytes.length;
    }

    @Override
    public CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) {
        target.writeBytes(bytes, (int)offset, len);
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) {
        source.readBytes(bytes, (int)offset, len);
        return CompletableFuture.completedFuture(null);
    }

    @Override
    public CompletableFuture<Void> flush(String exportName) {
        return CompletableFuture.completedFuture(null);
    }
}
