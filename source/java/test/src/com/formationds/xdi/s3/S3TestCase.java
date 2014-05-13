package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.formationds.util.Configuration;
import com.sun.security.auth.UserPrincipal;
import org.junit.Test;

import java.io.FileInputStream;

public class S3TestCase {
    //@Test
    public void testFdsImplementation() throws Exception {
        new Configuration(new String[0]);
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("AKIAINOGA4D75YX26VXQ", "/ZE1BUJ/vJ8BDESUvf5F3pib7lJW+pBa5FTakmjf"));
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "7VpLGuZy7VCKq2B/Z4yEOw=="));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:9999");
        Bucket bucket = client.createBucket("foo");
        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentType("image/jpg");
        client.putObject("foo", "image.jpg", new FileInputStream("/home/fabrice/Downloads/cat3.jpg"), metadata);
    }

    //@Test
    public void makeKey() throws Exception {
        String key = new AuthorizationToken(Authenticator.KEY, new UserPrincipal("fabrice")).getKey().toBase64();
        System.out.println(key);
    }

    @Test
    public void makeJUnitHappy() {

    }
}
