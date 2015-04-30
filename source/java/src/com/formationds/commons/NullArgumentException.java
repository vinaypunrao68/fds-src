package com.formationds.commons;

/**
 * An argument that is not allowed to be null is null.
 */
public class NullArgumentException extends IllegalArgumentException
{
    /**
     * Constructor.
     * 
     * @param argumentName The name of the argument that was null.
     */
    public NullArgumentException(String argumentName)
    {
        super("Illegal null for argument: " + argumentName);
    }
    
    /**
     * Serialization version.
     */
    private static final long serialVersionUID = 1L;
}
