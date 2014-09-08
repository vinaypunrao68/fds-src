package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.*;
import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.demo.Main;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.AuthenticationTokenTest;
import com.formationds.util.Configuration;
import com.formationds.util.Size;
import com.formationds.util.SizeUnit;
import junit.framework.Assert;
import org.apache.commons.io.IOUtils;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.Assert.assertEquals;

public class S3TestCase {
    //  @Test
    public void cleanupAwsVolumes() throws Exception {
        new Configuration("foo", new String[] {"--console"});
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("AKIAINOGA4D75YX26VXQ", "/ZE1BUJ/vJ8BDESUvf5F3pib7lJW+pBa5FTakmjf"));
        Arrays.stream(Main.VOLUMES)
                .forEach(v -> {
                    ObjectListing objectListing = client.listObjects(v);
                    String[] keys = objectListing.getObjectSummaries()
                            .stream()
                            .map(o -> o.getKey())
                            .toArray(i -> new String[i]);

                    DeleteObjectsResult result = client.deleteObjects(new DeleteObjectsRequest(v).withKeys(keys));
                    System.out.println("Deleted " + result.getDeletedObjects().size() + " keys in " + v);
                });
    }

    // @Test
    public void deleteMultipleObjects() throws Exception {
        new Configuration("foo", new String[] {"--console"});
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("admin", "yASvtWBUa8o2R+cr1jZqWEoCSmQ8kK/ofeZdC3zW4fiCKCKuToP13KoaObVMIHynMeE7BU8qCdNwkPSthgQsVw=="));
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "9VpLGuZy7VCKq2B/Z4yEOw=="));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");

        client.createBucket("foo");
        client.createBucket("bar");
        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentType("image/jpg");
        metadata.setHeader("Panda", "Pandas are friendly");
        client.putObject("foo", "foo.jpg", new FileInputStream("/home/fabrice/Downloads/cat3.jpg"), metadata);
        client.listBuckets().forEach(b -> System.out.println(b.getName()));
    }

    // @Test
    public void testFoo() throws Exception {
        new Configuration("foo", new String[] {"--console"});
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("admin", "a024f46c38be01f35f3ca39b0b1cd86588a33d634407d3f4ffcf3eec62306cc0c01212073b1906aa64584ba20e07ea8b549ee43a357e519a95fca2950b746c16"));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");
        client.listBuckets().forEach(b -> System.out.println(b.getName()));
    }

    //@Test
    public void testMount() throws Exception {
        TSocket amTransport = new TSocket("192.168.33.10", 9988);
        amTransport.open();
        AmService.Iface am = new AmService.Client(new TBinaryProtocol(amTransport));

        String omHost = "192.168.33.10";
        TSocket omTransport = new TSocket(omHost, 9090);
        omTransport.open();
        ConfigurationService.Iface config = new ConfigurationService.Client(new TBinaryProtocol(omTransport));

        config.createVolume("fds", "Volume1", new VolumeSettings(1024 * 4, VolumeType.BLOCK, new Size(20, SizeUnit.GB).totalBytes()), 0);
        Thread.sleep(2000);
        am.attachVolume("fds", "Volume1");
    }

    @Test
    public void testFdsImplementation() throws Exception {
        new Configuration("foo", new String[0]);
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("AKIAINOGA4D75YX26VXQ", "/ZE1BUJ/vJ8BDESUvf5F3pib7lJW+pBa5FTakmjf"));

        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "7VpLGuZy7VCKq2B/Z4yEOw=="));
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "9VpLGuZy7VCKq2B/Z4yEOw=="));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");
        Bucket bucket = client.createBucket("foo");
        ObjectMetadata metadata = new ObjectMetadata();
        //metadata.setContentType("image/jpg");
        client.putObject("foo", "foo.jpg", new FileInputStream("/home/fabrice/Downloads/cat3.jpg"), metadata);
        S3Object foo = client.getObject("foo", "foo.jpg");
        int read = IOUtils.copy(foo.getObjectContent(), new ByteArrayOutputStream());
        System.out.println("Read " + read + " bytes");
    }

    //@Test
    public void testHeadObject() throws Exception {
        AmazonS3Client client = makeAmazonS3Client();
        String bucketName = "foo";
        String objectName = "bar";
        byte[] data = new byte[] { 1, 2, 3, 4 };

        createBucketIdempotent(client, bucketName);
        ObjectMetadata obji = new ObjectMetadata();
        obji.setContentType("video/ogg");
        PutObjectResult result = client.putObject(bucketName, objectName, new ByteArrayInputStream(data), obji);
        ObjectMetadata objm = client.getObjectMetadata(bucketName, objectName);

        assertEquals(obji.getContentType(), objm.getContentType());
        assertEquals(result.getETag(), objm.getETag());
        assertEquals(data.length, objm.getContentLength());
    }

    //@Test
    public void testMultipartFdsImplementation() throws Exception {
        new Configuration("foo", new String[0]);
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("AKIAINOGA4D75YX26VXQ", "/ZE1BUJ/vJ8BDESUvf5F3pib7lJW+pBa5FTakmjf"));

        /*ClientConfiguration configuration = new ClientConfiguration()
                .withProxyHost("localhost")
                .withProxyPort(8008);*/

        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "7VpLGuZy7VCKq2B/Z4yEOw=="), configuration);
        AmazonS3Client client = makeAmazonS3Client();

        String bucketName = "foo";
        createBucketIdempotent(client, bucketName);

        String key = "wooky";
        String copySourceKey = "wookySrc";

        byte[] copyChunk = new byte[] { 12, 13, 14, 15 };
        PutObjectResult putObjectResult = client.putObject(bucketName, copySourceKey, new ByteArrayInputStream(copyChunk), new ObjectMetadata());

        InitiateMultipartUploadRequest req = new InitiateMultipartUploadRequest(bucketName, key);
        InitiateMultipartUploadResult initiateResult = client.initiateMultipartUpload(req);

        byte[][] chunks = new byte[][] {
                new byte[] { 0, 1, 2, 3 },
                new byte[] { 4, 5, 6, 7 },
                new byte[] { 8, 9, 10, 11 }
        };



        int ordinal = 0;
        List<PartETag> parts = new ArrayList<>();
        for(byte[] chunk : chunks) {
            UploadPartRequest upr = new UploadPartRequest()
                    .withBucketName(bucketName)
                    .withKey(key)
                    .withInputStream(new ByteArrayInputStream(chunk))
                    .withPartNumber(ordinal)
                    .withPartSize(chunk.length)
                    .withUploadId(initiateResult.getUploadId());

            UploadPartResult result = client.uploadPart(upr);
            parts.add(result.getPartETag());

            ordinal++;
        }

        CopyPartRequest uploadCopy = new CopyPartRequest()
                .withUploadId(initiateResult.getUploadId())
                .withSourceKey(copySourceKey)
                .withSourceBucketName(bucketName)
                .withDestinationKey(key)
                .withDestinationBucketName(bucketName)
                .withPartNumber(ordinal);
        CopyPartResult result = client.copyPart(uploadCopy);

        ListPartsRequest listPartsReq = new ListPartsRequest(bucketName, key, initiateResult.getUploadId());
        PartListing pl = client.listParts(listPartsReq);

        assertEquals(chunks.length + 1, pl.getParts().size());

        CompleteMultipartUploadRequest completeReq = new CompleteMultipartUploadRequest(bucketName, key, initiateResult.getUploadId(), parts);
        CompleteMultipartUploadResult completeResult = client.completeMultipartUpload(completeReq);

        S3Object obj = client.getObject(bucketName, key);

        ByteArrayOutputStream s3output = new ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        int read = 0;
        while((read = obj.getObjectContent().read(buf)) != -1)
            s3output.write(buf, 0, read);

        byte[] s3bytes = s3output.toByteArray();

        int idx = 0;
        for(byte[] chunk : chunks) {
            for(byte b : chunk) {
                assertEquals(b, s3bytes[idx++]);
            }
        }

        for(byte b : copyChunk) {
            assertEquals(b, s3bytes[idx++]);
        }
    }

    private void createBucketIdempotent(AmazonS3Client client, String bucketName) {
        try {
            client.createBucket(bucketName);
        } catch(AmazonS3Exception ex) {
            if(!ex.getErrorCode().equals(S3Failure.ErrorCode.BucketAlreadyExists.toString()))
                throw ex;
        }
    }

    private AmazonS3Client makeAmazonS3Client() {
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "9VpLGuZy7VCKq2B/Z4yEOw=="));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:8000");
        return client;
    }

    //@Test
    public void makeKey() throws Exception {
        String key = new AuthenticationToken(42, "foo").signature(AuthenticationTokenTest.SECRET_KEY);
        System.out.println(key);
    }

    @Test
    public void makeJUnitHappy() {

    }
}
