package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.workloads.Workload;

public final class CallChildWorkload extends AbstractOperation
{
    public CallChildWorkload(Workload workload)
    {
        if (workload == null) throw new NullArgumentException("workload");
        
        _workload = workload;
    }
    
    @Override
    public void accept(Endpoint endpoint,
                       WorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        _workload.runOn(endpoint, listener);
    }
    
    private final Workload _workload;
}
