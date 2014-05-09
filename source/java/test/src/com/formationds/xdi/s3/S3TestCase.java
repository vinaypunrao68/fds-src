package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.internal.ServiceUtils;
import com.amazonaws.services.s3.model.*;
import com.formationds.util.Configuration;
import org.junit.Test;

import java.io.FileInputStream;
import java.util.Date;

import static org.junit.Assert.assertEquals;

public class S3TestCase {
    //@Test
    public void testFdsImplementation() throws Exception {
        new Configuration(new String[0]);
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("AKIAINOGA4D75YX26VXQ", "/ZE1BUJ/vJ8BDESUvf5F3pib7lJW+pBa5FTakmjf"));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:9999");
        Bucket bucket = client.createBucket("foo");
        assertEquals(1, client.listBuckets().size());
        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentType("image/jpg");
        client.putObject("foo", "image2.jpg", new FileInputStream("/home/fabrice/Downloads/cat3.jpg"), metadata);
        //ObjectListing objectListing = client.listObjects("Bucket_Wild_Life");
        ObjectListing objectListing = client.listObjects("foo");
        assertEquals(1, objectListing.getObjectSummaries().size());
        for (S3ObjectSummary summary : objectListing.getObjectSummaries()) {
            S3Object object = client.getObject("foo", "image2.jpg");
            byte[] bytes = org.apache.commons.io.IOUtils.toByteArray(object.getObjectContent());
            assertEquals(29195, bytes.length);
            client.deleteObject("foo", summary.getKey());
        }
        assertEquals(0, client.listObjects("foo").getObjectSummaries().size());
        client.deleteBucket("foo");
        assertEquals(0, client.listBuckets().size());
    }

    @Test
    public void keepSillyJUnitHappy() throws Exception {
    }
}
