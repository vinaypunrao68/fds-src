package com.formationds.iodriver.operations;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.BaseHttpEndpoint;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.reporters.WorkloadEventListener;

public abstract class AbstractBaseHttpOperation<ConnectionT>
        extends AbstractOperation
        implements BaseHttpOperation<ConnectionT>
{
    @Override
    public void accept(Endpoint endpoint,
                       WorkloadEventListener listener) throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (listener == null) throw new NullArgumentException("listener");
        
        throw new UnsupportedOperationException(BaseHttpEndpoint.class.getName() + " is required.");
    }
    
    @Override
    public Class<ConnectionT> getConnectionType()
    {
        return _connectionType;
    }

    protected AbstractBaseHttpOperation(Class<ConnectionT> connectionType)
    {
        if (connectionType == null) throw new NullArgumentException("connectionType");
        
        _connectionType = connectionType;
    }
    
    private final Class<ConnectionT> _connectionType;
}
