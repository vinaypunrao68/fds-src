package com.formationds.util.async;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.CompletableFuture;


public class AsyncMessageDigest {
    private MessageDigest messageDigest;
    private LinkedList<String> lst;
    private CompletableFuture<Void> currentOperation;
    private final Object lock;
    private boolean finalized;
    private byte[] digest;

    public AsyncMessageDigest(MessageDigest messageDigest) {
        this.messageDigest = messageDigest;
        lock = new Object();
        currentOperation = CompletableFuture.completedFuture(null);
        digest = null;
        finalized = false;
    }

    public MessageDigest getMessageDigest() {
        return messageDigest;
    }

    public CompletableFuture<Void> update(ByteBuffer buffer) {
        final ByteBuffer buf = buffer.slice();
        synchronized (lock) {
            currentOperation = currentOperation.thenRunAsync(() -> {
                messageDigest.update(buf);
            });

            if(finalized)
                throw new RuntimeException("digest has already been finalized");

            return currentOperation;
        }
    }

    public CompletableFuture<byte[]> get() {
        final CompletableFuture<byte[]> result = new CompletableFuture<>();
        synchronized (lock) {
            finalized = true;

            currentOperation = currentOperation.thenRunAsync(() -> {
                if(digest == null)
                    digest = messageDigest.digest();

                result.complete(digest);
            });
        }
        return result;
    }
}
