package com.formationds.iodriver.operations;

public class ExecutionException extends Exception
{
    public ExecutionException()
    {
        this((String)null);
    }
    
    public ExecutionException(String message)
    {
        this(message, null);
    }
    
    public ExecutionException(Throwable cause)
    {
        this(null, cause);
    }
    
    public ExecutionException(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    private static final long serialVersionUID = 1L;
}
