package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.endpoints.Endpoint;
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
                       WorkloadContext context) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (context == null) throw new NullArgumentException("context");
        
        _workload.runOn(endpoint, context);
    }
    
    private final Workload _workload;
}
