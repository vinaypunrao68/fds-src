package com.formationds.iodriver.logging;

// This should only be thrown as an absolute last resort.
public class LoggingError extends Error
{
    public LoggingError()
    {
        this("An error occurred while logging an error. Unable to recover.");
    }
    
    public LoggingError(String message)
    {
        this(message, null);
    }
    
    public LoggingError(Throwable cause)
    {
        this(null, cause);
    }
    
    public LoggingError(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    private static final long serialVersionUID = 1L;
}
