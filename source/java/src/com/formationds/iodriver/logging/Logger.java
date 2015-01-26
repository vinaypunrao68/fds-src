package com.formationds.iodriver.logging;

public interface Logger
{
    void logError(String message, Exception ex);
    
    void logWarning(String message, Exception ex);
}
