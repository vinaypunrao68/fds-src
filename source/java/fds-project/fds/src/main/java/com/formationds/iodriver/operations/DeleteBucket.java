package com.formationds.iodriver.operations;

import com.amazonaws.AmazonClientException;
import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * Delete an S3 bucket.
 */
public class DeleteBucket extends S3Operation
{
    /**
     * Constructor.
     * 
     * @param bucketName The name of the bucket to delete.
     */
    public DeleteBucket(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        _bucketName = bucketName;
    }

    @Override
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        try
        {
            client.deleteBucket(_bucketName);
        }
        catch (AmazonClientException e)
        {
            throw new ExecutionException("Error deleting bucket.", e);
        }
    }

    /**
     * The name of the bucket to delete.
     */
    private final String _bucketName;
}
