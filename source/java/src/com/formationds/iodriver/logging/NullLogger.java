package com.formationds.iodriver.logging;


public final class NullLogger implements Logger
{
    public final static NullLogger INSTANCE;
    
    static
    {
        INSTANCE = new NullLogger();
    }
    
    @Override
    public void logError(String message, Exception ex)
    {
        // No-op.
    }
    
    @Override
    public void logWarning(String message, Exception ex)
    {
        // No-op.
    }
    
    private NullLogger() { }
}
