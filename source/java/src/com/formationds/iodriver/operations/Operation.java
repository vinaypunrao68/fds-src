package com.formationds.iodriver.operations;

import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;

public abstract class Operation
{
    public static final Operation NULL;
    
    public abstract void runOn(Endpoint endpoint) throws ExecutionException;
    
    static
    {
        NULL = new Operation()
        {
            @Override
            public void runOn(Endpoint endpoint)
            {
                if (endpoint == null) throw new NullArgumentException("endpoint");
                
                // No-op.
            }
        };
    }
}
