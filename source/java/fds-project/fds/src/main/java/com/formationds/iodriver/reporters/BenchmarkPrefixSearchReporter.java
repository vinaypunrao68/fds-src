package com.formationds.iodriver.reporters;

import java.io.Closeable;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.WorkloadContext;

public class BenchmarkPrefixSearchReporter implements Closeable
{
    public BenchmarkPrefixSearchReporter(PrintStream output, WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");
        
        _closed = new AtomicBoolean(false);
        _output = output;
        
    }
    
    @Override
    public void close()
    {
        if (_closed.compareAndSet(false, true))
        {
            
        }
    }
    
    private final AtomicBoolean _closed;
    
    private final PrintStream _output;
}
