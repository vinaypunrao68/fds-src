package com.formationds.smoketest;

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.*;
import org.apache.commons.io.IOUtils;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.log4j.PropertyConfigurator;
import org.json.JSONObject;
import org.junit.Ignore;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.security.SecureRandom;
import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import static org.junit.Assert.*;

/**
 * Copyright (c) 2014 Formation Data Systems.
 * All rights reserved.
 */

// We're just telling the unit test runner to ignore this, the class is ran SmokeTestRunner
@Ignore
public class SmokeTest {
    private static final String AMAZON_DISABLE_SSL = "com.amazonaws.sdk.disableCertChecking";
    private static final String FDS_AUTH_HEADER = "FDS-Auth";
    private final static String ADMIN_USERNAME = "admin";
    private static final String CUSTOM_METADATA_HEADER = "custom-metadata";

    private final String adminBucket;
    private final String userBucket;
    private final AmazonS3Client adminClient;
    private final AmazonS3Client userClient;
    private final byte[] randomBytes;

    public SmokeTest() throws Exception {
        System.setProperty(AMAZON_DISABLE_SSL, "true");
        String host = (String) System.getProperties().getOrDefault("fds.host", "localhost");
        String omUrl = "https://" + host + ":7443";
        turnLog4jOff();
        JSONObject adminUserObject = getObject(omUrl + "/api/auth/token?login=admin&password=admin", "");
        String adminToken = adminUserObject.getString("token");

        String tenantName = UUID.randomUUID().toString();
        long tenantId = doPost(omUrl + "/api/system/tenants/" + tenantName, adminToken).getLong("id");

        String userName = UUID.randomUUID().toString();
        String password = UUID.randomUUID().toString();
        long userId = doPost(omUrl + "/api/system/users/" + userName + "/" + password, adminToken).getLong("id");
        String userToken = getObject(omUrl + "/api/system/token/" + userId, adminToken).getString("token");
        doPut(omUrl + "/api/system/tenants/" + tenantId + "/" + userId, adminToken);
        adminBucket = UUID.randomUUID().toString();
        userBucket = UUID.randomUUID().toString();
        adminClient = s3Client(host, ADMIN_USERNAME, adminToken);
        userClient = s3Client(host, userName, userToken);
        adminClient.createBucket(adminBucket);
        userClient.createBucket(userBucket);
        randomBytes = new byte[4096];
        new SecureRandom().nextBytes(randomBytes);
    }

    @Test
    public void testMultipartUpload() {
        String key = UUID.randomUUID().toString();
        InitiateMultipartUploadResult initiateResult = userClient.initiateMultipartUpload(new InitiateMultipartUploadRequest(userBucket, key));
        int partCount = 5;

        List<PartETag> etags = IntStream.range(0, partCount)
                .map(new ConsoleProgress("Uploading parts", partCount))
                .mapToObj(i -> {
                    UploadPartRequest request = new UploadPartRequest()
                            .withBucketName(userBucket)
                            .withKey(key)
                            .withInputStream(new ByteArrayInputStream(randomBytes))
                            .withPartNumber(i)
                            .withUploadId(initiateResult.getUploadId())
                            .withPartSize(randomBytes.length);

                    UploadPartResult uploadPartResult = userClient.uploadPart(request);
                    return uploadPartResult.getPartETag();
                })
                .collect(Collectors.toList());

        CompleteMultipartUploadRequest completeRequest = new CompleteMultipartUploadRequest(userBucket, key, initiateResult.getUploadId(), etags);
        userClient.completeMultipartUpload(completeRequest);

        ObjectMetadata objectMetadata = userClient.getObjectMetadata(userBucket, key);
        assertEquals(4096 * partCount, objectMetadata.getContentLength());
    }

    @Test
    public void testPutGetDelete() {
        int count = 10;
        String prefix = UUID.randomUUID().toString();


        final PutObjectResult[] last = {null};
        IntStream.range(0, count)
                .map(new ConsoleProgress("Putting objects", count))
                .forEach(i -> {
                    ObjectMetadata objectMetadata = new ObjectMetadata();
                    Map<String, String> customMetadata = new HashMap<String, String>();
                    String key = prefix + "-" + i;
                    customMetadata.put(CUSTOM_METADATA_HEADER, key);
                    objectMetadata.setUserMetadata(customMetadata);
                    last[0] = userClient.putObject(userBucket, key, new ByteArrayInputStream(randomBytes), objectMetadata);
                });

        IntStream.range(0, count)
                .map(new ConsoleProgress("Getting objects", count))
                .forEach(i -> {
                    String key = prefix + "-" + i;
                    S3Object object = userClient.getObject(userBucket, key);
                    //assertEquals(key, object.getObjectMetadata().getUserMetaDataOf(CUSTOM_METADATA_HEADER));
                    //assertEquals(last[0].getContentMd5(), object.getObjectMetadata().getContentMD5());
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
                    String key = prefix + "-" + i;
                    userClient.deleteObject(userBucket, key);
                });
        assertEquals(0, userClient.listObjects(userBucket).getObjectSummaries().size());
    }

    @Test
    public void testUsersCannotAccessUnauthorizedObjects() {
        String key = UUID.randomUUID().toString();
        adminClient.putObject(adminBucket, key, new ByteArrayInputStream(randomBytes), new ObjectMetadata());
        try {
            userClient.getObject(adminBucket, key);
        } catch (AmazonS3Exception e) {
            return;
        } catch (Exception e) {
            e.printStackTrace();
            fail("Should have gotten an AmazonS3Exception!");
        }
    }

    @Test
    public void testUsersCannotAccessUnauthorizedVolumes() {
        try {
            userClient.listObjects(adminBucket);
        } catch (AmazonS3Exception e) {
            assertEquals("AccessDenied", e.getErrorCode());
            return;
        }

        fail("Should have gotten an AmazonS3Exception!");
    }

    @Test
    public void testAdminsCanSeeAllVolumes() {
        Set<String> bucketNames = adminClient.listBuckets()
                .stream()
                .map(b -> b.getName())
                .collect(Collectors.toSet());
        assertTrue(bucketNames.contains(adminBucket));
        assertTrue(bucketNames.contains(userBucket));
    }

    @Test
    public void testUsersCanOnlySeeTheirOwnTenantVolumes() {
        Set<String> bucketNames = userClient.listBuckets()
                .stream()
                .map(b -> b.getName())
                .collect(Collectors.toSet());
        assertFalse("Users should not see admin volumes!", bucketNames.contains(adminBucket));
    }

    private AmazonS3Client s3Client(String hostName, String userName, String token) {
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials(userName, token));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("https://" + hostName + ":8443");
        return client;
    }

    private JSONObject getObject(String url, String token) throws Exception {
        return new JSONObject(getString(url, token));
    }

    private String getString(String url, String token) throws Exception {
        HttpClient httpClient = new HttpClientFactory().makeHttpClient();
        HttpGet httpGet = new HttpGet(url);
        httpGet.setHeader(FDS_AUTH_HEADER, token);
        HttpResponse response = httpClient.execute(httpGet);
        return IOUtils.toString(response.getEntity().getContent());
    }

    private JSONObject doPost(String url, String token) throws Exception {
        HttpClient httpClient = new HttpClientFactory().makeHttpClient();
        HttpPost httpPost = new HttpPost(url);
        httpPost.setHeader(FDS_AUTH_HEADER, token);
        HttpResponse response = httpClient.execute(httpPost);
        return new JSONObject(IOUtils.toString(response.getEntity().getContent()));
    }

    private JSONObject doPut(String url, String token) throws Exception {
        HttpClient httpClient = new HttpClientFactory().makeHttpClient();
        HttpPut httpPut = new HttpPut(url);
        httpPut.setHeader(FDS_AUTH_HEADER, token);
        HttpResponse response = httpClient.execute(httpPut);
        return new JSONObject(IOUtils.toString(response.getEntity().getContent()));
    }

    private void turnLog4jOff() {
        Properties properties = new Properties();
        properties.put("log4j.rootCategory", "OFF, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        PropertyConfigurator.configure(properties);
    }

    private void turnLog4jOn() {
        Properties properties = new Properties();
        properties.put("log4j.rootCategory", "DEBUG, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        PropertyConfigurator.configure(properties);
    }
}
