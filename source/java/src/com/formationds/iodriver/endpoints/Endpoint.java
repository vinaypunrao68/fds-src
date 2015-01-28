package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.NullArgumentException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;

public abstract class Endpoint
{
    public static final Endpoint LOCALHOST;
    
    public abstract Endpoint copy();
    
    public abstract void exec(Operation operation) throws ExecutionException;
    
    protected abstract class CopyHelper { }
    
    protected Endpoint() { }
    
    protected Endpoint(CopyHelper helper)
    {
        if (helper == null) throw new NullArgumentException("helper");
    }
    
    static
    {
        // FIXME: Make a real localhost endpoint.
        LOCALHOST = null;
    }
}
