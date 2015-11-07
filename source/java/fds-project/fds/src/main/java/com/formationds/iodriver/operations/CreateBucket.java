package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.stream.Stream;

import com.amazonaws.AmazonClientException;
import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.S3Endpoint;

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
    public void accept(S3Endpoint endpoint,
                       AmazonS3Client client,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (context == null) throw new NullArgumentException("context");

        try
        {
            client.createBucket(_bucketName);
        }
        catch (AmazonClientException e)
        {
            throw new ExecutionException("Error creating bucket.", e);
        }
    }

    @Override
    protected Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("bucketName", _bucketName)));
    }
    
    /**
     * The name of the bucket to create.
     */
    private final String _bucketName;
}
