package com.formationds.xdi.io.async;

import java.util.concurrent.CompletableFuture;

public interface AsyncSequenceSource<T> {
    public CompletableFuture<AsyncSequence<T>> getSequence();
}
