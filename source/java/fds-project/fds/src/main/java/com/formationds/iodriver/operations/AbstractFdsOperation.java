package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;

public abstract class AbstractFdsOperation extends AbstractOperation implements FdsOperation
{
    @Override
    public void accept(Endpoint endpoint,
                       AbstractWorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        throw new UnsupportedOperationException(FdsEndpoint.class.getName() + " required.");
    }
}
