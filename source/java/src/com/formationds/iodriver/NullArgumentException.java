package com.formationds.iodriver;

public class NullArgumentException extends IllegalArgumentException
{
    public NullArgumentException(String argumentName)
    {
        super("Illegal null for argument: " + argumentName);
    }
    
    private static final long serialVersionUID = 1L;
}
