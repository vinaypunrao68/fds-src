package com.formationds.iodriver.operations;

import java.util.AbstractMap.SimpleImmutableEntry;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.stream.Stream;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public class AwaitGate extends AbstractOperation
{
    public AwaitGate(CyclicBarrier gate)
    {
        if (gate == null) throw new NullArgumentException("gate");
        
        _gate = gate;
    }
    
    public void accept(Endpoint endpoint,
                       WorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
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
