package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.log4j.Logger;

import java.util.Iterator;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingDeque;

public class DemoRunner {
    private static final Logger LOG = Logger.getLogger(DemoRunner.class);

    private String searchExpression;
    private int readParallelism;
    private int writeParallelism;
    private final ExecutorService executor;
    private Iterator<ImageResource> imageIterator;
    private final LinkedBlockingDeque<ImageResource> writeQueue;
    private StoredImage lastWritten;
    private ObjectStore objectStore;

    public DemoRunner(String searchExpression, int readParallelism, int writeParallelism, ObjectStore objectStore) {
        this.objectStore = objectStore;
        LOG.info("New DemoRunner, q=" + searchExpression);
        this.searchExpression = searchExpression;
        this.readParallelism = readParallelism;
        this.writeParallelism = writeParallelism;
        executor = Executors.newCachedThreadPool();
        writeQueue = new LinkedBlockingDeque<>(1000);
        lastWritten = null;
    }

    public void start() {
        imageIterator = new CircularIterator<>(() -> new FlickrStream(searchExpression, 500));

        executor.submit(new Stoppable("Enqueue image", () -> {
            Thread.sleep(500);
            ImageResource next = imageIterator.next();
            writeQueue.put(next);
        }));

        for (int i = 0; i < writeParallelism; i++) {
            executor.submit(new Stoppable("Writing image", () -> {
                Thread.sleep(500);
                ImageResource resource = writeQueue.take();
                lastWritten = objectStore.getImageWriter().write(resource);
            }));
        }

        for (int i = 0; i < readParallelism; i++) {
            executor.submit(new Stoppable("Reading image", () -> {
                Thread.sleep(500);
                if (lastWritten != null) {
                    StoredImage r = objectStore.getImageReader().read(lastWritten);
                }
            }));
        }

        executor.shutdown();
    }

    public java.util.Optional<ImageResource> peekWriteQueue() {
        return lastWritten == null ? Optional.<ImageResource>empty() : Optional.of(lastWritten.getImageResource());
    }

    public ObjectStore getObjectStore() {
        return objectStore;
    }

    public java.util.Optional<ImageResource> peekReadQueue() {
        return peekWriteQueue();
    }

    public BucketStats readCounts() {
        return objectStore.getImageReader().counts();
    }

    public BucketStats writeCounts() {
        return objectStore.getImageWriter().consumeCounts();
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
