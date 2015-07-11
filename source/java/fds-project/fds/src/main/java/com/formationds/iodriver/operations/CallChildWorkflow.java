package com.formationds.iodriver.operations;

import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.workloads.Workload;

public final class CallChildWorkflow<
        EndpointT extends Endpoint<EndpointT, CallChildWorkflow<EndpointT, ?>>,
        WorkloadT extends Workload<EndpointT, ? super CallChildWorkflow<EndpointT, ?>>>
        extends Operation<CallChildWorkflow<EndpointT, WorkloadT>, EndpointT>
{
    public CallChildWorkflow(Workload<? super EndpointT, ?> workload, Observer<SimpleImmutableEntry<)
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
