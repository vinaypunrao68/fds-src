package com.formationds.iodriver.logging;

import java.io.PrintStream;
import java.util.Arrays;

import com.formationds.iodriver.NullArgumentException;

public final class ConsoleLogger implements Logger
{
    private final String _indent;

    public ConsoleLogger()
    {
        _indent = "\t";
    }

    private static PrintStream getErrorOutput()
    {
        return System.out;
    }

    private static void printStackTrace(PrintStream out,
                                        Iterable<? extends StackTraceElement> stack,
                                        String indent)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");
        if (indent == null) throw new NullArgumentException("indent");

        for (StackTraceElement frame : stack)
        {
            out.println(indent + " at " + frame.getClassName() + "." + frame.getMethodName() + "("
                        + frame.getFileName() + ":" + frame.getLineNumber() + ")");
        }
    }

    @Override
    public void logError(String message, Exception ex)
    {
        logError(getErrorOutput(), message, ex);
    }

    @Override
    public void logWarning(String message, Exception ex)
    {
        logWarning(getErrorOutput(), message, ex);
    }

    private void log(PrintStream out, String text, Iterable<? extends StackTraceElement> stack)
    {
        try
        {
            if (out == null)
            {
                out = getErrorOutput();
                logWarning("out is null.");
            }

            if (text == null && stack == null)
            {
                out.println("No error information provided.");
                printStackTrace(out, Thread.currentThread().getStackTrace());
            }
            else
            {
                if (text != null)
                {
                    out.println(text);
                }
                if (stack != null)
                {
                    printStackTrace(out, stack);
                }
            }
        }
        catch (Exception ex)
        {
            throw new LoggingError(ex);
        }
    }

    private void logError(PrintStream out, String message, Exception ex)
    {
        logError(out, message, ex == null ? null : ex.getStackTrace());
    }

    private void logError(PrintStream out,
                          String message,
                          Iterable<? extends StackTraceElement> stack)
    {
        log(out, message == null ? null : ("[ERROR]: " + message), stack);
    }

    private void logError(PrintStream out, String message, StackTraceElement[] stack)
    {
        logError(out, message, stack == null ? null : Arrays.asList(stack));
    }

    private void logWarning(PrintStream out, String message)
    {
        logWarning(out, message, (Iterable<StackTraceElement>)null);
    }

    private void logWarning(PrintStream out, String message, Exception ex)
    {
        logWarning(out, message, ex == null ? null : ex.getStackTrace());
    }

    private void logWarning(PrintStream out, String message, StackTraceElement[] stack)
    {
        logWarning(out, message, stack == null ? null : Arrays.asList(stack));
    }

    private void logWarning(PrintStream out,
                            String message,
                            Iterable<? extends StackTraceElement> stack)
    {
        log(out, message == null ? null : ("[WARN]: " + message), stack);
    }

    private void logWarning(String message)
    {
        logWarning(getErrorOutput(), message);
    }

    private void printStackTrace(PrintStream out, Iterable<? extends StackTraceElement> stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, stack, _indent);
    }

    private void printStackTrace(PrintStream out, StackTraceElement[] stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, Arrays.asList(stack));
    }
}
