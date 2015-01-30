package com.formationds.iodriver.logging;

import java.io.PrintStream;
import java.util.Arrays;

import com.formationds.commons.NullArgumentException;
import com.google.common.base.Strings;

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

    private void log(PrintStream out,
                     String text,
                     String errorType,
                     Iterable<? extends StackTraceElement> stack,
                     int indentDepth)
    {
        try
        {
            String indent = Strings.repeat(_indent, indentDepth);

            if (out == null)
            {
                out = getErrorOutput();
                logWarning("out is null.");
            }

            if (text == null && errorType == null && stack == null)
            {
                out.println(indent + "No error information provided.");
                printStackTrace(out, Thread.currentThread().getStackTrace(), indent);
            }
            else
            {
                if (text != null)
                {
                    out.println(indent + text);
                }
                if (errorType != null)
                {
                    out.println(indent + errorType);
                }
                if (stack != null)
                {
                    printStackTrace(out, stack, indent);
                }
            }
        }
        catch (Exception ex)
        {
            throw new LoggingError(ex);
        }
    }

    private void logError(PrintStream out, String message, Throwable t)
    {
        if (t == null)
        {
            logError(out, message, null, (StackTraceElement[])null, 0);
        }
        else
        {
            int indent = 0;
            while (t != null)
            {
                logError(out,
                         message,
                         t.getClass().getName() + ": " + t.getMessage(),
                         t.getStackTrace(),
                         indent);
                t = t.getCause();
                message = "Caused by:";
                ++indent;
            }
        }
    }

    private void logError(PrintStream out,
                          String message,
                          String errorType,
                          Iterable<? extends StackTraceElement> stack,
                          int indentDepth)
    {
        log(out, message == null ? null : ("[ERROR]: " + message), errorType, stack, indentDepth);
    }

    private void logError(PrintStream out,
                          String message,
                          String errorType,
                          StackTraceElement[] stack,
                          int indentDepth)
    {
        logError(out, message, errorType, stack == null ? null : Arrays.asList(stack), indentDepth);
    }

    private void logWarning(PrintStream out, String message)
    {
        logWarning(out, message, null);
    }

    private void logWarning(PrintStream out, String message, Throwable t)
    {
        if (t == null)
        {
            logWarning(out, message, null, (StackTraceElement[])null, 0);
        }
        else
        {
            int indent = 0;
            while (t != null)
            {
                logWarning(out,
                           message,
                           t.getClass().getName() + ": " + t.getMessage(),
                           t.getStackTrace(),
                           indent);
                t = t.getCause();
                message = "Caused by: ";
                ++indent;
            }
        }
    }

    private void logWarning(PrintStream out,
                            String message,
                            String errorType,
                            StackTraceElement[] stack,
                            int indentDepth)
    {
        logWarning(out,
                   message,
                   errorType,
                   stack == null ? null : Arrays.asList(stack),
                   indentDepth);
    }

    private void logWarning(PrintStream out,
                            String message,
                            String errorType,
                            Iterable<? extends StackTraceElement> stack,
                            int indentDepth)
    {
        log(out, message == null ? null : ("[WARN]: " + message), errorType, stack, indentDepth);
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

    private void printStackTrace(PrintStream out, StackTraceElement[] stack, String indent)
    {
        if (out == null) throw new NullArgumentException("out");
        if (stack == null) throw new NullArgumentException("stack");

        printStackTrace(out, Arrays.asList(stack));
    }
}
