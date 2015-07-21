package com.formationds.iodriver;

/**
 * Thrown when there was an error detected in system configuration.
 */
public class ConfigurationException extends Exception
{
    /**
     * Constructor.
     * 
     * @param message A descriptive error message, or {@code null} (not recommended).
     */
    public ConfigurationException(String message)
    {
        this(message, null);
    }
    
    /**
     * Constructor.
     * 
     * @param message A descriptive error message, or {@code null} (not recommended).
     * @param cause The proximate cause of this error, or {@code null} if there was none.
     */
    public ConfigurationException(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    /**
     * Identifies serialization version.
     */
    private static final long serialVersionUID = 1L;
}
