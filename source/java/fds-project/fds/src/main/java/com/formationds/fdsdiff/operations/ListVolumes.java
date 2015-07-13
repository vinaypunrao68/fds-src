package com.formationds.fdsdiff.operations;

import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.OmEndpoint;
import com.formationds.iodriver.operations.AbstractFdsOperation;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class ListVolumes extends AbstractFdsOperation implements FdsOperation
{
    @Override
    public void accept(FdsEndpoint endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        OmEndpoint om = endpoint.getOmEndpoint();
        
        om.visit(this, listener);
    }
}
