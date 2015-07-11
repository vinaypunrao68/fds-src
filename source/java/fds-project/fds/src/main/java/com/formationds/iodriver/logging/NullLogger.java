package com.formationds.iodriver.logging;

/**
 * No-op logger. Does nothing.
 */
public final class NullLogger implements Logger
{
    /**
     * Singleton instance.
     */
    public final static NullLogger INSTANCE;

    static
    {
        INSTANCE = new NullLogger();
    }

    @Override
    public void logDebug(String message)
    {
        // No-op.
    }
    
    @Override
    public void logError(String message, Throwable t)
    {
        // No-op.
    }

    @Override
    public void logWarning(String message, Throwable t)
    {
        // No-op.
    }

    /**
     * Constructor.
     */
    private NullLogger()
    {}
}
