package com.formationds.iodriver.endpoints;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.FdsOperation;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public final class FdsEndpoint extends Endpoint<FdsEndpoint, FdsOperation>
{
    public FdsEndpoint(OrchestrationManagerEndpoint omEndpoint)
    {
        super(FdsOperation.class);
        
        if (omEndpoint == null) throw new NullArgumentException("omEndpoint");
        
        _omEndpoint = omEndpoint;
    }

    @Override
    public FdsEndpoint copy()
    {
        _CopyHelper copyHelper = new _CopyHelper();
        
        return new FdsEndpoint(copyHelper);
    }

    @Override
    public void doVisit(FdsOperation operation,
                        AbstractWorkflowEventListener listener) throws ExecutionException
    {
        operation.accept(this, listener);
    }

    public OrchestrationManagerEndpoint getOmEndpoint()
    {
        return _omEndpoint;
    }
    
    private final class _CopyHelper extends Endpoint<FdsEndpoint, FdsOperation>.CopyHelper
    {
        public final OrchestrationManagerEndpoint omEndpoint = FdsEndpoint.this._omEndpoint;
    }
    
    private FdsEndpoint(_CopyHelper copyHelper)
    {
        super(copyHelper);
        
        _omEndpoint = copyHelper.omEndpoint;
    }
    
    private final OrchestrationManagerEndpoint _omEndpoint;
}
