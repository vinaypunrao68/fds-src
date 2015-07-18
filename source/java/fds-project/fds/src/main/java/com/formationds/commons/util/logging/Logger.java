package com.formationds.commons.util.logging;

/**
 * Implementing classes can log.
 */
public interface Logger
{
    /**
     * Log a debugging message.
     * 
     * @param message The message to log.
     */
    void logDebug(String message);
    
    /**
     * Log an error.
     * 
     * @param message Detailed error message explaining context.
     * @param t The error.
     */
    void logError(String message, Throwable t);

    void logError(String message);
    
    /**
     * Log a warning.
     * 
     * @param message Detailed message explaining context.
     * @param t The non-fatal error.
     */
    void logWarning(String message, Throwable t);
}
