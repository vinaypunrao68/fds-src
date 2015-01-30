package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.VerificationReporter;

public abstract class S3Operation extends Operation<S3Operation, S3Endpoint>
{
    public abstract void exec(S3Endpoint endpoint,
                              AmazonS3Client client,
                              VerificationReporter reporter) throws ExecutionException;

    @Override
    public void accept(S3Endpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");

        endpoint.visit(this);
    }
}
