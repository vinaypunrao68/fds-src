package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.services.s3.AmazonS3Client;
import com.amazonaws.services.s3.S3ClientOptions;
import com.amazonaws.services.s3.model.S3Object;
import org.apache.commons.io.IOUtils;

public class S3ImageReader extends ImageReader {
    private AmazonS3Client client;

    public S3ImageReader(BasicAWSCredentials credentials) {
        client = new AmazonS3Client(credentials);
    }

    public S3ImageReader(String host, int port) {
        this(new BasicAWSCredentials("foo", "bar"));
        client.setS3ClientOptions(new S3ClientOptions().withPathStyleAccess(true));
        client.setEndpoint("http://" + host + ":" + port);
    }

    @Override
    protected StoredImage read(StoredImage storedImage) throws Exception {
        Thread.sleep(500);
        S3Object object = client.getObject(storedImage.getVolumeName(), storedImage.getImageResource().getId());
        IOUtils.toByteArray(object.getObjectContent());
        increment(storedImage.getVolumeName());
        return storedImage;

    }
}
