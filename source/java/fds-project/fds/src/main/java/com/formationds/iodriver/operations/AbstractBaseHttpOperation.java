package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;

public abstract class AbstractBaseHttpOperation<ConnectionT>
        extends AbstractOperation
        implements BaseHttpOperation<ConnectionT>
{
    @Override
    public void accept(Endpoint endpoint,
                       AbstractWorkflowEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        endpoint.visit(this, listener);
    }
}
