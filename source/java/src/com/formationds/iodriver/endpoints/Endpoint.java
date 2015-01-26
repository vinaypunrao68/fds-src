package com.formationds.iodriver.endpoints;

import com.formationds.iodriver.NotImplementedException;
import com.formationds.iodriver.operations.ExecutionException;
import com.formationds.iodriver.operations.Operation;

public abstract class Endpoint
{
    public static final Endpoint LOCALHOST;
    
    public Endpoint()
    {
        // Fix the fixme's before taking this out.
        throw new NotImplementedException();
    }
    
    public abstract Endpoint copy();
    
    public abstract void exec(Operation operation) throws ExecutionException;
    
    protected abstract class CopyHelper { }
    
    static
    {
        // FIXME: Make a real localhost endpoint.
        LOCALHOST = null;
    }
}
