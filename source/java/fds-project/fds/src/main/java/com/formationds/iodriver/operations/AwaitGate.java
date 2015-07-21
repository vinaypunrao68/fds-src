package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.stream.Stream;

import com.amazonaws.services.s3.AmazonS3Client;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public class AwaitGate extends S3Operation
{
    public AwaitGate(CyclicBarrier gate)
    {
        if (gate == null) throw new NullArgumentException("gate");
        
        _gate = gate;
    }
    
    @Override
    public void exec(S3Endpoint endpoint,
                     AmazonS3Client client,
                     AbstractWorkflowEventListener reporter) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (client == null) throw new NullArgumentException("client");
        if (reporter == null) throw new NullArgumentException("reporter");
        
        try
        {
            _gate.await();
        }
        catch (InterruptedException e)
        {
            Thread.currentThread().interrupt();
        }
        catch (BrokenBarrierException e)
        {
            throw new ExecutionException("Error waiting on gate.", e);
        }
    }
    
    @Override
    public Stream<SimpleImmutableEntry<String, String>> toStringMembers()
    {
        return Stream.concat(super.toStringMembers(),
                             Stream.of(memberToString("gate", _gate)));
    }
    
    private final CyclicBarrier _gate;
}
