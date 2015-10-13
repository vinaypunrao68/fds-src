package com.formationds.commons.util.logging;

/**
 * This should only be thrown as an absolute last resort. It's expected that this will crash the
 * program, and somehow be reported somewhere.
 */
public class LoggingError extends Error
{
    /**
     * Constructor.
     */
    public LoggingError()
    {
        this("An error occurred while logging an error. Unable to recover.");
    }
    
    /**
     * Constructor.
     * 
     * @param message Message explaining context.
     */
    public LoggingError(String message)
    {
        this(message, null);
    }
    
    /**
     * Constructor.
     * 
     * @param cause Proximate cause.
     */
    public LoggingError(Throwable cause)
    {
        this(null, cause);
    }
    
    /**
     * Constructor.
     * 
     * @param message Message explaining context.
     * @param cause Proximate cause.
     */
    public LoggingError(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    /**
     * Serialization format.
     */
    private static final long serialVersionUID = 1L;
}
