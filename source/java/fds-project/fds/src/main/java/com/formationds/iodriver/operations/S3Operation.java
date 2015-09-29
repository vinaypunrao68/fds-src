package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

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
     * @param listener Report progress here.
     * 
     * @throws ExecutionException when an error occurs.
     */
    public abstract void accept(S3Endpoint endpoint,
                                AmazonS3Client client,
                                WorkloadEventListener listener) throws ExecutionException;
    
    @Override
    public void accept(Endpoint endpoint,
                       WorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        throw new UnsupportedOperationException(S3Endpoint.class.getName() + " is required.");
    }
}
