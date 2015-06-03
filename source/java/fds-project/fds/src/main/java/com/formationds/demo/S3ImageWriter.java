package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.ObjectMetadata;

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.net.URL;

public class S3ImageWriter extends ImageWriter {

    private AmazonS3Client client;

    public S3ImageWriter(BasicAWSCredentials credentials, String[] volumeNames, BucketStats bucketStats) {
        super(volumeNames, bucketStats);
        client = new AmazonS3Client(credentials);
    }

    public S3ImageWriter(String host, int port, String[] volumeNames, BucketStats bucketStats) {
        this(new BasicAWSCredentials("foo", "bar"), volumeNames, bucketStats);
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://" + host + ":" + port);
    }

    @Override
    public StoredImage write(ImageResource resource) {
        try {
            String volume = randomVolume();
            URL url = new URL(resource.getUrl());
            try (InputStream inputStream = new BufferedInputStream(url.openConnection().getInputStream(), 1024 * 10)) {
                ObjectMetadata metadata = new ObjectMetadata();
                metadata.setContentType("image/jpg");
                client.putObject(volume, resource.getName(), inputStream, metadata);
                increment(volume);
                return new StoredImage(resource, volume);
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

    }
}
