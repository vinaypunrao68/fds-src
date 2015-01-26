package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;
import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;

public abstract class S3Operation extends Operation
{
    public abstract void exec(AmazonS3Client client) throws ExecutionException;
    
    @Override
    public void runOn(Endpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (!(endpoint instanceof S3Endpoint))
        {
            throw new IllegalArgumentException("endpoint must be an instance of " + S3Endpoint.class.getName());
        }
        
        runOn((S3Endpoint)endpoint);
    }
    
    public void runOn(S3Endpoint endpoint) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        
        endpoint.exec(this);
    }
}
