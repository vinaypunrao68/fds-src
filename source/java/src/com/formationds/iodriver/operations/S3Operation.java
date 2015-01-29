package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;

public abstract class S3Operation extends Operation<S3Operation, S3Endpoint>
{
    public abstract void exec(AmazonS3Client client) throws ExecutionException;
    
    @Override
    public void accept(S3Endpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        endpoint.visit(this);
    }
}
