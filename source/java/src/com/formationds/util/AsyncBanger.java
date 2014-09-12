package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.event.ProgressEvent;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.transfer.TransferManager;
import com.amazonaws.services.s3.transfer.Upload;

import java.io.ByteArrayInputStream;
import java.security.SecureRandom;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ForkJoinPool;
import java.util.stream.IntStream;

public class AsyncBanger {
    private static final String BUCKET_NAME = "pink";

    public static void main(String[] args) throws Exception {
        SecureRandom random = new SecureRandom();
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("admin", "c1131660703d9ab2e157f086c926a6cb99af165e0f573db8b44d29843a1980c483bad14097df1fb9c9c6106dfbf69c533dfc0dba887727d1288667cb07ad8eb0"));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");
        TransferManager tm = new TransferManager(client);
        //client.createBucket(BUCKET_NAME);
        int concurrency = 100;



        CompletableFuture[] cfs = IntStream.range(0, concurrency)
                .mapToObj(i -> {
                    CompletableFuture<Boolean> c = new CompletableFuture<>();
                    c.supplyAsync(() -> {
                        byte[] bytes = new byte[4096];
                        random.nextBytes(bytes);
                        Upload upload = tm.upload(BUCKET_NAME, UUID.randomUUID().toString(), new ByteArrayInputStream(bytes), new ObjectMetadata());
                        upload.addProgressListener((ProgressEvent progressEvent) -> {
                            switch (progressEvent.getEventCode()) {
                                case ProgressEvent.COMPLETED_EVENT_CODE:
                                    c.complete(Boolean.FALSE);
                                    break;

                                case ProgressEvent.FAILED_EVENT_CODE:
                                    c.completeExceptionally(new Exception("Upload failed with error code " + progressEvent.getEventCode()));
                                    break;

                                default:
                                    break;
                            }
                        });
                        return false;
                    }, ForkJoinPool.commonPool());
                    return c;
                })
                .toArray(i -> new CompletableFuture[i]);

        CompletableFuture.allOf(cfs).get();
    }
}
