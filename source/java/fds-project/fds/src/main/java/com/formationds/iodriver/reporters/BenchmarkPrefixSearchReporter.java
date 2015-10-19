package com.formationds.iodriver.reporters;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.PrefixSearch;
import com.formationds.iodriver.operations.GetObjects;

public class BenchmarkPrefixSearchReporter implements Closeable
{
    public BenchmarkPrefixSearchReporter(PrintStream output, WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");
        
        _closed = new AtomicBoolean(false);
        _output = output;
        _prefixSearchToken = context.subscribe(PrefixSearch.class, this::_onPrefixSearch);
    }
    
    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _prefixSearchToken.close();
        }
    }
    
    private final AtomicBoolean _closed;
    
    private final PrintStream _output;
    
    private final Closeable _prefixSearchToken;
    
    private void _onPrefixSearch(PrefixSearch event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        PrefixSearch.Data data = event.getData();
        GetObjects operation = data.getOperation();
        
        _output.println("[" + event.getTimestamp() + "]"
                        + " Bucket: " + javaString(operation.getBucketName())
                        + " Prefix: " + javaString(operation.getPrefix())
                        + " Delimiter: " + javaString(operation.getDelimiter())
                        + " Results: " + data.getResultCount()
                        + " Elapsed: " + data.getElapsed()
                        + " Per Object: " + (data.getResultCount() == 0
                                ? "∞"
                                : data.getElapsed().dividedBy(data.getResultCount())));
    }
}
