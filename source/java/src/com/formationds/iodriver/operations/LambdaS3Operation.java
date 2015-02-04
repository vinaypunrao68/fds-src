package com.formationds.iodriver.operations;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.WorkflowEventListener;

public class LambdaS3Operation extends S3Operation
{
    @FunctionalInterface
    public interface Delegate
    {
        void exec(S3Endpoint endpoint, AmazonS3Client client, WorkflowEventListener reporter) throws ExecutionException;
    }

    public LambdaS3Operation(Runnable action)
    {
        this(createDelegate(action));
    }
    
    public LambdaS3Operation(Delegate delegate)
    {
        if (delegate == null) throw new NullArgumentException("delegate");
        
        _delegate = delegate;
    }
    
    @Override
    public void exec(S3Endpoint endpoint, AmazonS3Client client, WorkflowEventListener reporter) throws ExecutionException
    {
        _delegate.exec(endpoint, client, reporter);
    }

    private final Delegate _delegate;
    
    private static Delegate createDelegate(Runnable action)
    {
        if (action == null) throw new NullArgumentException("action");
        
        return (e, c, r) -> action.run();
    }
}
