package com.formationds.web.toolkit;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.servlet.ReadListener;
import javax.servlet.ServletInputStream;
import java.io.IOException;
import java.util.concurrent.CompletableFuture;


public class ServletInputWapper implements ReadListener {
    private ServletInputStream inputStream;
    private final Object lock;
    private CompletableFuture<Boolean> readReady;

    public ServletInputWapper(ServletInputStream inputStream) {
        this.inputStream = inputStream;
        inputStream.setReadListener(this);
        lock = new Object();
    }

    public void onDataAvailable() throws IOException {
        synchronized (lock) {
            if(readReady != null)
                readReady.complete(true);
        }
    }

    @Override
    public void onAllDataRead() throws IOException {
        synchronized (lock) {
            if(readReady != null)
                readReady.complete(false);
        }
    }

    @Override
    public void onError(Throwable throwable) {
        synchronized (lock) {
            if(readReady != null)
                readReady.completeExceptionally(throwable);
        }
    }

    public CompletableFuture<Boolean> dataAvailable() {
        synchronized (lock) {
            if(this.readReady != null) {
                CompletableFuture<Boolean> cf = new CompletableFuture<>();
                cf.completeExceptionally(new Exception("parallel reads are not supported"));
                return cf;
            }

            if(inputStream.isReady()) {
                return CompletableFuture.completedFuture(!inputStream.isFinished());
            }

            readReady = new CompletableFuture<>();
            return readReady;
        }
    }

    public CompletableFuture<Integer> read(byte[] array) {
        return read(array, 0, array.length);
    }

    // NB: only one outstanding read() is allowed at a time
    public CompletableFuture<Integer> read(byte[] array, int offset, int length) {
        if(length == 0)
            return CompletableFuture.completedFuture(0);

        return readInner(array, offset, length, 0);
    }

    private CompletableFuture<Integer> readInner(byte[] array, int offset, int length, int read) {
        return dataAvailable().<Integer>thenCompose(hasData -> {
            if (!hasData) {
                return CompletableFuture.completedFuture(read == 0 ? -1 : read);
            } else {
                try {
                    synchronized (lock) {
                        int readCount = 0;
                        while (inputStream.isReady() && readCount < length) {
                            int r = inputStream.read(array, offset + readCount, length - readCount);
                            if (r == -1)
                                break;
                            readCount += r;
                        }

                        if (read + readCount == length)
                            return CompletableFuture.completedFuture(length);
                        else
                            return readInner(array, offset + readCount, length - readCount, read + readCount);
                    }
                } catch (IOException e) {
                    CompletableFuture<Integer> result = new CompletableFuture<>();
                    result.completeExceptionally(e);
                    return result;
                }
            }
        });
    }
}
