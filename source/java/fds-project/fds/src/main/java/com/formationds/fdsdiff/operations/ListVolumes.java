package com.formationds.fdsdiff.operations;

import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class ListVolumes extends FdsOperation
{
    @Override
    public void accept(FdsEndpoint endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        OrchestrationManagerEndpoint om = endpoint.getOmEndpoint();
        
        om.doVisit(this, listener);
    }
}
