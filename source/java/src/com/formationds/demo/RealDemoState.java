package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;
import org.joda.time.Duration;

import java.util.Optional;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public class RealDemoState implements DemoState {
    private static final ObjectStoreType DEFAULT_OBJECT_STORE = ObjectStoreType.apiS3;
    private DateTime lastAccessed;
    private DemoRunner demoRunner;
    private DemoConfig demoConfig;

    public RealDemoState(Duration timeToLive, DemoConfig demoConfig) {
        this.demoConfig = demoConfig;
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

    @Override
    public void setSearchExpression(String searchExpression) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(searchExpression, demoRunner.getReadParallelism(), demoRunner.getWriteParallelism(), demoRunner.getObjectStore());
            demoRunner = newRunner;
            demoRunner.start();
        } else {
            ObjectStore objectStore = new ObjectStore(demoConfig, DEFAULT_OBJECT_STORE);
            demoRunner = new DemoRunner(searchExpression, 0, 0, objectStore);
            demoRunner.start();
        }
    }

    @Override
    public Optional<String> getCurrentSearchExpression() {
        lastAccessed = DateTime.now();
        return demoRunner == null? Optional.<String>empty() : Optional.of(demoRunner.getQuery());
    }

    @Override
    public Optional<ImageResource> peekReadQueue() {
        return peekWriteQueue();
    }

    @Override
    public Optional<ImageResource> peekWriteQueue() {
        lastAccessed = DateTime.now();
        return demoRunner == null ? Optional.empty() : demoRunner.peekReadQueue();
    }

    @Override
    public int getReadThrottle() {
        lastAccessed = DateTime.now();
        return demoRunner == null? 0 : demoRunner.getReadParallelism();
    }

    @Override
    public void setReadThrottle(int value) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(demoRunner.getQuery(), value, demoRunner.getWriteParallelism(), demoRunner.getObjectStore());
            demoRunner = newRunner;
            demoRunner.start();
        }
    }

    @Override
    public int getWriteThrottle() {
        lastAccessed = DateTime.now();
        return demoRunner == null? 0 : demoRunner.getWriteParallelism();
    }

    @Override
    public void setWriteThrottle(int value) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            DemoRunner newRunner = new DemoRunner(demoRunner.getQuery(), demoRunner.getReadParallelism(), value, demoRunner.getObjectStore());
            demoRunner = newRunner;
            demoRunner.start();
        }
    }

    @Override
    public void setObjectStore(ObjectStoreType type) {
        lastAccessed = DateTime.now();
        if (demoRunner != null) {
            demoRunner.tearDown();
            ObjectStore objectStore = new ObjectStore(demoConfig, type);
            DemoRunner newRunner = new DemoRunner(demoRunner.getQuery(), demoRunner.getReadParallelism(), demoRunner.getWriteParallelism(), objectStore);
            demoRunner = newRunner;
            demoRunner.start();
        }
    }

    @Override
    public ObjectStoreType getObjectStore() {
        lastAccessed = DateTime.now();
        return demoRunner == null? DEFAULT_OBJECT_STORE : demoRunner.getObjectStore().getType();
    }

    @Override
    public Counts consumeReadCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new Counts(): demoRunner.consumeReadCounts();
    }

    @Override
    public Counts consumeWriteCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new Counts(): demoRunner.consumeWriteCounts();

    }
}
