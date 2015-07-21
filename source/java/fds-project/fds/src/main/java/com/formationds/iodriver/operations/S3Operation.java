package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

/**
 * An operation on an S3 endpoint.
 */
public abstract class S3Operation extends Operation<S3Operation, S3Endpoint>
{
    /**
     * Perform the final operation.
     * 
     * @param endpoint The endpoint to run the operation on.
     * @param client The S3 client to use to run the operation.
     * @param reporter Report progress here.
     * 
     * @throws ExecutionException when an error occurs.
     */
    public abstract void exec(S3Endpoint endpoint,
                              AmazonS3Client client,
                              AbstractWorkflowEventListener reporter) throws ExecutionException;

    @Override
    // @eclipseFormat:off
    public void accept(S3Endpoint endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        endpoint.visit(this, listener);
    }
}
