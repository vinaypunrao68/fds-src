package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;

/**
 * An operation on an S3 endpoint.
 */
public abstract class S3Operation extends AbstractOperation
{
    /**
     * Perform the final operation.
     * 
     * @param endpoint The endpoint to run the operation on.
     * @param client A client that operates on {@code endpoint}.
     * 
     * @throws ExecutionException when an error occurs.
     */
    public abstract void accept(S3Endpoint endpoint,
                                AmazonS3Client client,
                                WorkloadContext context) throws ExecutionException;
    
    @Override
    public void accept(Endpoint endpoint,
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (context == null) throw new NullArgumentException("context");
        
        throw new UnsupportedOperationException(S3Endpoint.class.getName() + " is required.");
    }
}
