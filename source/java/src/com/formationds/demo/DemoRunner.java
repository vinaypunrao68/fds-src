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
    private ImageReader reader;
    private ImageWriter writer;
    private int readParallelism;
    private int writeParallelism;
    private final ExecutorService executor;
    private Iterator<ImageResource> imageIterator;
    private final LinkedBlockingDeque<ImageResource> writeQueue;
    private StoredImage lastWritten;

    public DemoRunner(String searchExpression, ImageReader reader, ImageWriter writer, int readParallelism, int writeParallelism) {
        LOG.info("New DemoRunner, q=" + searchExpression);
        this.searchExpression = searchExpression;
        this.reader = reader;
        this.writer = writer;
        this.readParallelism = readParallelism;
        this.writeParallelism = writeParallelism;
        executor = Executors.newCachedThreadPool();
        writeQueue = new LinkedBlockingDeque<>(1000);
        lastWritten = null;
    }

    public void start() {
        imageIterator = new CircularIterator<>(() -> new FlickrStream(searchExpression, 500));
        executor.submit(() -> {
            while (imageIterator.hasNext()) {
                try {
                    ImageResource next = imageIterator.next();
                    writeQueue.put(next);
                } catch (InterruptedException e) {
                    //throw new RuntimeException(e);
                }
            }
        });

        for (int i = 0; i < writeParallelism; i++) {
            executor.submit((Runnable) () -> {
                while (true) {
                    try {
                        ImageResource resource = writeQueue.take();
                        lastWritten = writer.write(resource);
                        LOG.debug("Wrote " + resource.getUrl());
                    } catch (Exception e) {
                        LOG.info("Error writing :", e);
                    }
                }
            });
        }

        for (int i = 0; i < readParallelism; i++) {
            executor.submit((Runnable)() -> {
                while (true) {
                    try {
                        if (lastWritten != null) {
                            StoredImage r = reader.read(lastWritten);
                            LOG.debug("Read " + lastWritten);
                        }
                    } catch (Exception e) {
                        LOG.info("Error reading image", e);
                        //break;
                    }
                }
            });
        }
        executor.shutdown();
    }

    public java.util.Optional<ImageResource> peekWriteQueue() {
        return lastWritten == null ? Optional.<ImageResource>empty() : Optional.of(lastWritten.getImageResource());
    }

    public java.util.Optional<ImageResource> peekReadQueue() {
        return peekWriteQueue();
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
