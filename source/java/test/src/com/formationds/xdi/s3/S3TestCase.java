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
import com.formationds.apis.VolumeConnector;
import com.formationds.apis.VolumePolicy;
import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.formationds.util.Configuration;
import com.sun.security.auth.UserPrincipal;
import org.apache.commons.io.IOUtils;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;

public class S3TestCase {
    //@Test
    public void deleteMultipleObjects() throws Exception {
        new Configuration("foo", new String[0]);
        AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "7VpLGuZy7VCKq2B/Z4yEOw=="));
        //AmazonS3Client client = new AmazonS3Client(new BasicAWSCredentials("fabrice", "9VpLGuZy7VCKq2B/Z4yEOw=="));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://localhost:9999");

        Bucket bucket = client.createBucket("foo");
        ObjectMetadata metadata = new ObjectMetadata();
        //metadata.setContentType("image/jpg");
        client.putObject("foo", "foo.jpg", new FileInputStream("/home/fabrice/Downloads/cat3.jpg"), metadata);
        DeleteObjectsResult result = client.deleteObjects(new DeleteObjectsRequest("foo").withKeys("foo.jpg", "bar.jpg"));
        System.out.println(result);
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

        config.createVolume("fds", "Volume1", new VolumePolicy(1024 * 4, VolumeConnector.CINDER));
        Thread.sleep(2000);
        am.attachVolume("fds", "Volume1");
    }

    //@Test
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
    public void makeKey() throws Exception {
        String key = new AuthorizationToken(Authenticator.KEY, new UserPrincipal("fabrice")).getKey().toBase64();
        System.out.println(key);
    }

    @Test
    public void makeJUnitHappy() {

    }
}
