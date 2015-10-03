package com.formationds.xdi.io.async;

import java.util.Optional;
import java.util.concurrent.CompletableFuture;

public interface AsyncSequence<T> {
    public T getCurrent();
    public CompletableFuture<Optional<AsyncSequence<T>>> getNext();
}
