package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

public abstract class S3Operation extends Operation<S3Operation, S3Endpoint>
{
    public abstract void exec(S3Endpoint endpoint,
                              AmazonS3Client client,
                              WorkflowEventListener reporter) throws ExecutionException;

    @Override
    // @eclipseFormat:off
    public void accept(S3Endpoint endpoint,
                       WorkflowEventListener listener) throws ExecutionException
    // @eclipseFormat:on
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");

        endpoint.visit(this, listener);
    }
}
