package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;

import java.util.concurrent.CompletableFuture;

public interface NbdServerOperations {
    boolean exists(String exportName);
    long size(String exportName);
    CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) throws Exception;
    CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception;
    CompletableFuture<Void> flush(String exportName);
}
