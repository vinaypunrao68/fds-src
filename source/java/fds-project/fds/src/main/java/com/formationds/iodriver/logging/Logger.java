package com.formationds.iodriver.logging;

/**
 * Implementing classes can log.
 */
public interface Logger
{
    /**
     * Log an error.
     * 
     * @param message Detailed error message explaining context.
     * @param t The error.
     */
    void logError(String message, Throwable t);

    /**
     * Log a warning.
     * 
     * @param message Detailed message explaining context.
     * @param t The non-fatal error.
     */
    void logWarning(String message, Throwable t);
}
