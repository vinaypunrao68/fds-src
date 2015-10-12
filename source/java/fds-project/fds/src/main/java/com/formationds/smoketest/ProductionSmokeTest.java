package com.formationds.smoketest;

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.formationds.util.Configuration;
import org.apache.commons.io.IOUtils;
import org.junit.Ignore;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.security.SecureRandom;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import static org.junit.Assert.*;

/**
 * Copyright (c) 2014 Formation Data Systems, Inc.
 */
@Ignore
public class ProductionSmokeTest {

    private AmazonS3Client client;
    private String bucketName;
    private byte[] randomBytes;

    public ProductionSmokeTest() {
        new Configuration("foo", new String[]{"--console"});
        client = new AmazonS3Client(new BasicAWSCredentials("anichols", "cacf0a325b5e8bd357fb04f7b53680cee7fd8fcec2bdd53033c9754c8ec53bef4541790ca2415ddfce79f341ba0c3502376a44146e8351c4eb40d27cf7719ee4"));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("https://us-east.formationds.com");
    }

    //@Test
    public void testDeleteBucket() {
        client.listBuckets().forEach(b -> System.out.println(b.getName()));
    }

    //@Test
    public void testDeleteFullBucket() {
        int count = 10;
        IntStream.range(0, count)
                .map(new ConsoleProgress("Putting objects", count))
                .forEach(i -> {
                    ObjectMetadata objectMetadata = new ObjectMetadata();
                    Map<String, String> customMetadata = new HashMap<String, String>();
                    String key = Integer.toString(i);
                    objectMetadata.setUserMetadata(customMetadata);
                    client.putObject(bucketName, key, new ByteArrayInputStream(randomBytes), objectMetadata);
                });

        assertTrue(listBuckets().contains(bucketName));
        client.deleteBucket(bucketName);
        assertFalse(listBuckets().contains(bucketName));
    }

    private Set<String> listBuckets() {
        return client.listBuckets()
                .stream()
                .map(b -> b.getName())
                .collect(Collectors.toSet());
    }

    //@Test
    public void testPutGet() {
        final PutObjectResult[] last = {null};
        int count = 20;
        IntStream.range(0, count)
                .map(new ConsoleProgress("Putting objects", count))
                .forEach(i -> {
                    ObjectMetadata objectMetadata = new ObjectMetadata();
                    Map<String, String> customMetadata = new HashMap<String, String>();
                    String key = Integer.toString(i);
                    objectMetadata.setUserMetadata(customMetadata);
                    last[0] = client.putObject(bucketName, key, new ByteArrayInputStream(randomBytes), objectMetadata);
                });

        IntStream.range(0, count)
                .map(new ConsoleProgress("Getting objects", count))
                .forEach(i -> {
                    String key = Integer.toString(i);
                    S3Object object = client.getObject(bucketName, key);
                    assertEquals(last[0].getETag(), object.getObjectMetadata().getETag());
                    try {
                        assertArrayEquals(randomBytes, IOUtils.toByteArray(object.getObjectContent()));
                    } catch (IOException e) {
                        e.printStackTrace();
                        fail("Error reading object");
                    }
                });

        IntStream.range(0, count)
                .map(new ConsoleProgress("Deleting objects", count))
                .forEach(i -> {
                    String key = Integer.toString(i);
                    client.deleteObject(bucketName, key);
                });
        assertEquals(0, client.listObjects(bucketName).getObjectSummaries().size());
    }

    @Test
    public void keepJunitHappy() {

    }

    //@Before
    public void setUp() {
        bucketName = UUID.randomUUID().toString();
        client.createBucket(bucketName);
        randomBytes = new byte[4096];
        new SecureRandom().nextBytes(randomBytes);

    }

    //@After
    public void tearDown() {
        try {
            client.deleteBucket(bucketName);
        } catch (Exception e) {

        }
    }
}
