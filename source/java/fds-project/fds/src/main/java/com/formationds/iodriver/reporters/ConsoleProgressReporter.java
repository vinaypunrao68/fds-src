package com.formationds.iodriver.reporters;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Observable;
import com.formationds.iodriver.operations.Operation;

/**
 * Reports workload progress on the system console.
 */
public final class ConsoleProgressReporter implements Closeable
{
    /**
     * Constructor.
     * 
     * @param output Stream to output to, usually {@code System.out}.
     */
    public ConsoleProgressReporter(PrintStream output,
                                   Observable<Operation> operationExecuted)
    {
        if (output == null) throw new NullArgumentException("output");
        if (operationExecuted == null) throw new NullArgumentException("operationExecuted");

        _closed = new AtomicBoolean(false);
        _output = output;
        _operationExecuted = operationExecuted.register(this::onOperationExecuted);
    }

    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _operationExecuted.close();
        }
    }

    private void onOperationExecuted(Operation operation)
    {
        if (operation == null) throw new NullArgumentException("operationExecuted");
        
        _output.println("Executing: " + operation);
    }
    
    /**
     * Whether this object has been closed.
     */
    private final AtomicBoolean _closed;

    /**
     * Operation executed event token.
     */
    private final Closeable _operationExecuted;
    
    /**
     * Output.
     */
    private final PrintStream _output;
}
