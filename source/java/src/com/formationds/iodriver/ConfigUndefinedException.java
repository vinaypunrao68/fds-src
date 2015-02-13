package com.formationds.iodriver;

/**
 * Thrown when a needed configuration is not present.
 */
public class ConfigUndefinedException extends ConfigurationException
{
    /**
     * Constructor.
     *
     * @param message A descriptive error message, or {@code null} (not recommended).
     */
    public ConfigUndefinedException(String message)
    {
        super(message);
    }

    /**
     * Identifies serialization version.
     */
    private static final long serialVersionUID = 1L;
}
