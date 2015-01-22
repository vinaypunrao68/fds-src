package com.formationds.iodriver;

public final class Workload
{
    public static final Workload NULL;

    public Workload()
    {
        _rootOperation = Operation.NULL;
    }
    
    public final Operation getRootOperation()
    {
        return _rootOperation;
    }
    
    public final void runOn(Endpoint endpoint)
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        
        getRootOperation().runOn(endpoint);
    }
    
    static
    {
        NULL = new Workload();
    }
    
    private Operation _rootOperation;
}
