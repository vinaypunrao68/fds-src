package com.formationds.web.toolkit;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.servlet.ServletOutputStream;
import javax.servlet.WriteListener;
import java.io.IOException;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.CompletableFuture;

public class ServletOutputWrapper implements WriteListener {
    private ServletOutputStream stream;
    private final Queue<Write> writeQueue;
    private Write current;

    public ServletOutputWrapper(ServletOutputStream stream) {
        this.stream = stream;
        writeQueue = new LinkedList<>();
        stream.setWriteListener(this);
    }

    public interface WriteConsumer<T> {
        void accept(T val) throws IOException;
    }

    private class Write {
        public Write(WriteConsumer<ServletOutputStream> writeAction, CompletableFuture<Void> handle) {
            this.writeAction = writeAction;
            this.handle = handle;
        }
        public WriteConsumer<ServletOutputStream> writeAction;
        public CompletableFuture<Void> handle;
    }

    public CompletableFuture<Void> outAsync(WriteConsumer<ServletOutputStream> writeAction) throws IOException {
        CompletableFuture<Void> cf = new CompletableFuture<>();
        Write w = new Write(writeAction, cf);

        synchronized (writeQueue) {
            if(current == null) {
                current = w;
                w.writeAction.accept(stream);
            } else {
                writeQueue.add(w);
            }
        }

        return cf;
    }

    @Override
    public void onWritePossible() throws IOException {
        synchronized (writeQueue) {
            if(current != null) {
                current.handle.complete(null);
                if(!writeQueue.isEmpty()) {
                    current = writeQueue.remove();
                    current.writeAction.accept(stream);
                }
            }
        }
    }

    @Override
    public void onError(Throwable throwable) {
        synchronized (writeQueue) {
            if(current != null) {
                current.handle.completeExceptionally(null);
                while(!writeQueue.isEmpty()) {
                    current = writeQueue.remove();
                    try {
                        current.writeAction.accept(stream);
                        break;
                    } catch(IOException ex) {
                        current.handle.completeExceptionally(ex);
                    }
                }
            }
        }
    }
}
