package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.workloads.Workload;

public final class CallChildWorkload<
        EndpointT extends Endpoint<EndpointT, ? super CallChildWorkload<EndpointT, WorkloadT>>,
        WorkloadT extends Workload<? extends EndpointT,
                                   ? super CallChildWorkload<EndpointT, WorkloadT>>>
        extends Operation<CallChildWorkload<EndpointT, WorkloadT>, EndpointT>
{
    public CallChildWorkload(Workload<? super EndpointT, ?> workload)
    {
        if (workload == null) throw new NullArgumentException("workload");
        
        _workload = workload;
    }
    
    @Override
    public void accept(EndpointT visitor,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        _workload.runOn(visitor, listener);
    }
    
    private final Workload<? super EndpointT, ?> _workload;
}
