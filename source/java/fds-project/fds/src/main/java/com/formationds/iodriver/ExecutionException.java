package com.formationds.iodriver;

import com.formationds.iodriver.operations.Operation;

/**
 * An error occurred while executing an {@link Operation}.
 */
public class ExecutionException extends Exception
{
    /**
     * Constructor with empty message. Not recommended.
     */
    public ExecutionException()
    {
        this((String)null);
    }
    
    /**
     * Constructor.
     * 
     * @param message Detail message explaining this error.
     */
    public ExecutionException(String message)
    {
        this(message, null);
    }
    
    /**
     * Constructor.
     * 
     * @param cause The proximate cause of this error.
     */
    public ExecutionException(Throwable cause)
    {
        this(null, cause);
    }
    
    /**
     * Constructor.
     * 
     * @param message Detail message explaining this error.
     * @param cause The proximate cause of this error.
     */
    public ExecutionException(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    /**
     * Serialization version of this class.
     */
    private static final long serialVersionUID = 1L;
}
