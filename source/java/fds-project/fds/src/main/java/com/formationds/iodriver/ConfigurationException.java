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
        super(message, null);
    }
    
    /**
     * Identifies serialization version.
     */
    private static final long serialVersionUID = 1L;
}
