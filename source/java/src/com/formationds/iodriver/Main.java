package com.formationds.iodriver;

import java.io.PrintStream;
import java.util.Arrays;

public final class Main
{
    static
    {
        INDENT = "\t";
    }

    public static void main(String[] args)
    {
        try
        {
            Driver driver = new Driver();
            driver.runWorkload();
        }
        catch (Exception ex)
        {
            logError("Unexpected exception.", ex);
        }
    }

    private Main()
    {
        throw new UnsupportedOperationException("Trying to instantiate a utility class.");
    }

    private static final String INDENT;

    private static PrintStream getErrorOutput()
    {
        return System.out;
    }

    private static void log(PrintStream out,
                            String text,
                            Iterable<? extends StackTraceElement> stack)
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

    private static void logError(String message, Exception ex)
    {
        logError(getErrorOutput(), message, ex);
    }

    private static void logError(PrintStream out, String message, Exception ex)
    {
        logError(out, message, ex == null ? null : ex.getStackTrace());
    }

    private static void logError(PrintStream out, String message, StackTraceElement[] stack)
    {
        logError(out, message, stack == null ? null : Arrays.asList(stack));
    }

    private static void logError(PrintStream out,
                                 String message,
                                 Iterable<? extends StackTraceElement> stack)
    {
        log(out, message == null ? null : ("[ERROR]: " + message), stack);
    }

    private static void logWarning(String message)
    {
        logWarning(getErrorOutput(), message);
    }

    private static void logWarning(PrintStream out, String message)
    {
        logWarning(out, message, (Iterable<StackTraceElement>)null);
    }

    private static void logWarning(PrintStream out,
                                   String message,
                                   Iterable<? extends StackTraceElement> stack)
    {
        log(out, message == null ? null : ("[WARN]: " + message), stack);
    }

    private static void printStackTrace(PrintStream out, StackTraceElement[] stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, Arrays.asList(stack));
    }

    private static void
            printStackTrace(PrintStream out, Iterable<? extends StackTraceElement> stack)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, stack, INDENT);
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
}
