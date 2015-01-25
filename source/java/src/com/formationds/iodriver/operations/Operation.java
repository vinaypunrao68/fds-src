package com.formationds.iodriver;

public abstract class Operation
{
    public static final Operation NULL;
    
    public abstract void runOn(Endpoint endpoint);
    
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
