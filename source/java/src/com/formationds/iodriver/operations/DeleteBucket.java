package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.iodriver.NullArgumentException;

public class DeleteBucket extends S3Operation
{
    public DeleteBucket(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        _bucketName = bucketName;
    }

    @Override
    public void exec(AmazonS3Client client)
    {
        if (client == null) throw new NullArgumentException("client");

        client.deleteBucket(_bucketName);
    }

    private final String _bucketName;
}
