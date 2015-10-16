package com.formationds.iodriver;

public class CommandLineConfigurationException extends ConfigurationException
{
    public CommandLineConfigurationException(String message)
    {
        this(message, null);
    }
    
    public CommandLineConfigurationException(String message, Throwable cause)
    {
        super(message, cause);
    }
    
    private static final long serialVersionUID = 1L;
}
