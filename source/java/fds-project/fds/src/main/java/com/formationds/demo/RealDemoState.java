package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import org.apache.log4j.Logger;
import org.joda.time.DateTime;
import org.joda.time.Duration;

import java.util.Arrays;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class RealDemoState implements DemoState {
    private static final Logger LOG = Logger.getLogger(RealDemoState.class);

    private static final ObjectStoreType DEFAULT_OBJECT_STORE = ObjectStoreType.apiS3;
    public static final int MAX_AGE_SECONDS = 10;
    private DateTime lastAccessed;
    private DemoRunner demoRunner;
    private DemoConfig demoConfig;

    public RealDemoState(Duration timeToLive, DemoConfig demoConfig) {
        this.demoConfig = demoConfig;
        lastAccessed = DateTime.now();

        //createLocalVolumes(demoConfig);
        //createAwsVolumes(demoConfig);

        ExecutorService scavenger = Executors.newCachedThreadPool();
        scavenger.submit((Runnable) () -> {
            while (true) {
                try {
                    Thread.sleep(5 * 1000);
                    if (new Duration(lastAccessed, DateTime.now()).isLongerThan(timeToLive)) {
                        if (demoRunner != null) {
                            demoRunner.tearDown();
                            LOG.info("Took down demo runner");
                            demoRunner = null;
                        }
                    }
                } catch (Exception e) {
                    LOG.error("Error scavenging demo runner", e);
                }
            }
        });
        scavenger.shutdown();
        LOG.info("Demo state started");
    }

    private void createAwsVolumes(DemoConfig demoConfig) {
        AmazonS3Client client = new AmazonS3Client(demoConfig.getCreds());
        createVolumes(client, demoConfig.getVolumeNames());
    }

    private void createLocalVolumes(DemoConfig demoConfig){
        AmazonS3Client client = new AmazonS3Client(demoConfig.getCreds());
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://" + demoConfig.getAmHost() + ":" + demoConfig.getS3Port());
        createVolumes(client, demoConfig.getVolumeNames());
    }

    private void createVolumes(AmazonS3Client client, String[] bucketNames) {
        Arrays.stream(bucketNames)
                .forEach(b -> {
                    try {
                        client.createBucket(b);
                    } catch (Exception e) {
                        LOG.info("Error creating buckets - can probably be ignored", e);
                    }
                });
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
    public BucketStats readCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new BucketStats(MAX_AGE_SECONDS, TimeUnit.SECONDS): demoRunner.readCounts();
    }

    @Override
    public BucketStats writeCounts() {
        lastAccessed = DateTime.now();
        return demoRunner == null? new BucketStats(MAX_AGE_SECONDS, TimeUnit.SECONDS): demoRunner.writeCounts();

    }
}
