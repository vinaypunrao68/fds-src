package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * An operation on an S3 endpoint.
 */
public abstract class S3Operation extends AbstractOperation
{
    /**
     * Perform the final operation.
     * 
     * @param endpoint The endpoint to run the operation on.
     * @param client The S3 client to use to run the operation.
     * @param listener Report progress here.
     * 
     * @throws ExecutionException when an error occurs.
     */
    public abstract void accept(S3Endpoint endpoint,
                                AmazonS3Client client,
                                AbstractWorkflowEventListener listener) throws ExecutionException;
    
    @Override
    public void accept(Endpoint endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        throw new UnsupportedOperationException(S3Endpoint.class.getName() + " is required.");
    }
}
