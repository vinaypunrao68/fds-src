package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Iterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingDeque;

public class DemoRunner {
    private String searchExpression;
    private ImageReader reader;
    private ImageWriter writer;
    private int readParallelism;
    private int writeParallelism;
    private final ExecutorService executor;
    private Iterator<ImageResource> imageIterator;
    private final LinkedBlockingDeque<ImageResource> writeQueue;
    private Single<ImageResource> lastRead;
    private Single<ImageResource> lastWritten;

    public DemoRunner(String searchExpression, ImageReader reader, ImageWriter writer, int readParallelism, int writeParallelism) {
        this.searchExpression = searchExpression;
        this.reader = reader;
        this.writer = writer;
        this.readParallelism = readParallelism;
        this.writeParallelism = writeParallelism;
        executor = Executors.newCachedThreadPool();
        writeQueue = new LinkedBlockingDeque<>(1000);
        lastRead = new Single<>();
        lastWritten = new Single<>();
    }

    public void start() {
        imageIterator = new CircularIterator<>(() -> new FlickrStream(searchExpression, 500));
        executor.submit(() -> {
            while (imageIterator.hasNext()) {
                try {
                    writeQueue.put(imageIterator.next());
                } catch (InterruptedException e) {
                    //throw new RuntimeException(e);
                }
            }
        });

        for (int i = 0; i < writeParallelism; i++) {
            executor.submit(() -> {
                while (true) {
                    try {
                        ImageResource resource = writeQueue.take();
                        lastWritten.set(resource);
                        writer.write(resource);
                    } catch (InterruptedException e) {
                        break;
                    }
                }

                return true;
            });
        }

        for (int i = 0; i < readParallelism; i++) {
            executor.submit(() -> {
                while (true) {
                    try {
                        lastRead.set(reader.readOne());
                    } catch (Exception e) {
                        break;
                    }
                }
                return true;
            });
        }
        executor.shutdown();
    }

    public ImageResource peekWriteQueue() {
        return lastWritten.get();
    }

    public ImageResource peekReadQueue() {
        return lastRead.get();
    }

    public Counts consumeReadCounts() {
        return reader.consumeCounts();
    }

    public Counts consumeWriteCounts() {
        return writer.consumeCounts();
    }

    public void tearDown() {
        executor.shutdownNow();
    }

    public String getQuery() {
        return searchExpression;
    }

    public int getReadParallelism() {
        return readParallelism;
    }

    public int getWriteParallelism() {
        return writeParallelism;
    }
}
