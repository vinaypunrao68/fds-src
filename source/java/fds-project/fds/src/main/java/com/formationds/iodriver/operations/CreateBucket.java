package com.formationds.iodriver.operations;

import com.amazonaws.AmazonClientException;
import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

/**
 * Create an S3 bucket.
 */
public class CreateBucket extends S3Operation
{
    /**
     * Constructor.
     * 
     * @param bucketName The name of the bucket to create.
     */
    public CreateBucket(String bucketName)
    {
        if (bucketName == null) throw new NullArgumentException("bucketName");

        _bucketName = bucketName;
    }

    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, WorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");

        try
        {
            client.createBucket(_bucketName);
        }
        catch (AmazonClientException e)
        {
            throw new ExecutionException("Error creating bucket.", e);
        }
    }

    /**
     * The name of the bucket to create.
     */
    private final String _bucketName;
}
