package com.formationds.commons.util.logging;

import java.io.PrintStream;
import java.util.Arrays;

import com.formationds.commons.NullArgumentException;

/**
 * Logs to the system console.
 */
public final class ConsoleLogger implements Logger
{
    @Override
    public void logDebug(String message)
    {
        logDebug(getStandardOutput(), message);
    }
    
    @Override
    public void logError(String message, Throwable t)
    {
        logError(getErrorOutput(), message, t);
    }
    
    @Override
    public void logError(String message)
    {
        logError(message, null);
    }

    @Override
    public void logWarning(String message, Throwable t)
    {
        logWarning(getErrorOutput(), message, t);
    }

    /**
     * Print a bare stack trace.
     * 
     * @param out Output destination.
     * @param stack The stack trace to print.
     */
    public static void printStackTrace(PrintStream out,
                                       Iterable<? extends StackTraceElement> stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        for (StackTraceElement frame : stack)
        {
            out.println("\tat " + frame.getClassName() + "." + frame.getMethodName() + "("
                        + frame.getFileName() + ":" + frame.getLineNumber() + ")");
        }
    }

    /**
     * Log something. All fields MAY be {@code null}, any that can be non-{@code null} should be.
     * 
     * @param out Output destination.
     * @param text Detail text.
     * @param level Error level text, goes in [] at the beginning of the line.
     * @param t Cause of this log entry.
     */
    private void log(PrintStream out, String text, String level, Throwable t)
    {
        try
        {
            String formattedLevel = "[" + (level == null ? "ERROR" : level) + "]: ";

            if (out == null)
            {
                out = getErrorOutput();
                logWarning("out is null.");
            }

            if (text == null && t == null)
            {
                out.println(formattedLevel + "No error information provided.");
                printStackTrace(out, Thread.currentThread().getStackTrace());
            }
            else
            {
                if (text != null)
                {
                    out.println(formattedLevel + text);
                }
                if (t != null)
                {
                    t.printStackTrace(out);
                }
            }
        }
        catch (Exception ex)
        {
            throw new LoggingError(ex);
        }
    }

    /**
     * Log something. All fields MAY be {@code null}, any that can be non-{@code null} should be.
     * 
     * @param out Output destination.
     * @param text Detail text.
     * @param level Error level text, goes in [] at the beginning of the line.
     */
    private void log(PrintStream out, String text, String level)
    {
        log(out, text, level, null);
    }
    
    /**
     * Log a debug message.
     * 
     * @param out Output destination.
     * @param message The message to log.
     */
    private void logDebug(PrintStream out, String message)
    {
        log(out, message, "DEBUG");
    }
    
    /**
     * Log an error.
     * 
     * @param out Output destination.
     * @param message Error message.
     * @param t Proximate cause.
     */
    private void logError(PrintStream out, String message, Throwable t)
    {
        log(out, message, "ERROR", t);
    }

    /**
     * Log a warning.
     * 
     * @param message User message.
     */
    private void logWarning(String message)
    {
        logWarning(getErrorOutput(), message);
    }

    /**
     * Log a warning.
     * 
     * @param out Output destination.
     * @param message User message.
     */
    private void logWarning(PrintStream out, String message)
    {
        logWarning(out, message, null);
    }

    /**
     * Log a warning.
     * 
     * @param out Output destination.
     * @param message User message.
     * @param t Proximate cause.
     */
    private void logWarning(PrintStream out, String message, Throwable t)
    {
        log(out, message, "WARN", t);
    }

    /**
     * Print a bare stack trace.
     * 
     * @param out Output destination.
     * @param stack The stack to print.
     */
    private void printStackTrace(PrintStream out, StackTraceElement[] stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, Arrays.asList(stack));
    }

    /**
     * Get the output destination for errors.
     * 
     * @return A destination, probably {@code System.out}.
     */
    private static PrintStream getErrorOutput()
    {
        return System.out;
    }
    
    /**
     * Get the output destination for standard messages.
     * 
     * @return A destination, probably {@code System.out}.
     */
    private static PrintStream getStandardOutput()
    {
        return System.out;
    }
}
