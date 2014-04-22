package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.xdi.Xdi;
import org.joda.time.DateTime;
import org.joda.time.Duration;

import java.util.Optional;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class TransientState {
    private DateTime lastAccessed;
    private DemoRunner demoRunner;
    private final ImageReader reader;
    private final ImageWriter writer;

    public TransientState(Duration timeToLive, Xdi xdi) {
        reader = new FdsImageReader(xdi);
        writer = new FdsImageWriter(xdi);

        lastAccessed = DateTime.now();

        ScheduledExecutorService scavenger = Executors.newSingleThreadScheduledExecutor();
        scavenger.scheduleAtFixedRate(() -> {
            if (new Duration(lastAccessed, DateTime.now()).isLongerThan(timeToLive)) {
                if (demoRunner != null) {
                    demoRunner.tearDown();
                    demoRunner = null;
                }
            }
        }, 0, 5, TimeUnit.SECONDS);
        scavenger.shutdown();
    }

    public void setSearchExpression(String searchExpression) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(searchExpression, reader, writer, demoRunner.getReadParallelism(), demoRunner.getWriteParallelism());
            demoRunner = newRunner;
            demoRunner.start();
        } else {
            demoRunner = new DemoRunner(searchExpression, reader, writer, 1, 1);
            demoRunner.start();
        }
    }

    public Optional<String> getCurrentSearchExpression() {
        lastAccessed = DateTime.now();
        return demoRunner == null? Optional.<String>empty() : Optional.of(demoRunner.getQuery());
    }

    public Optional<ImageResource> peekReadQueue() {
        lastAccessed = DateTime.now();
        return demoRunner == null? Optional.empty() : Optional.of(demoRunner.peekReadQueue());
    }

    public Optional<ImageResource> peekWriteQueue() {
        lastAccessed = DateTime.now();
        return demoRunner == null ? Optional.empty() : Optional.of(demoRunner.peekReadQueue());
    }

    public int getReadThrottle() {
        lastAccessed = DateTime.now();
        return demoRunner == null? 0 : demoRunner.getReadParallelism();
    }

    public void setReadThrottle(int value) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(demoRunner.getQuery(), reader, writer, value, demoRunner.getWriteParallelism());
            demoRunner = newRunner;
            demoRunner.start();
        }
    }

    public int getWriteThrottle() {
        lastAccessed = DateTime.now();
        return demoRunner == null? 0 : demoRunner.getWriteParallelism();
    }

    public void setWriteThrottle(int value) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(demoRunner.getQuery(), reader, writer, demoRunner.getReadParallelism(), value);
            demoRunner = newRunner;
            demoRunner.start();
        }
    }

    public Counts consumeReadCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new Counts(): demoRunner.consumeReadCounts();
    }

    public Counts consumeWriteCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new Counts(): demoRunner.consumeWriteCounts();

    }
}
