package com.formationds.iodriver.reporters;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.Event;
import com.formationds.iodriver.events.OperationExecuted;
import com.formationds.iodriver.events.VolumeAdded;
import com.formationds.iodriver.events.VolumeModified;
import com.formationds.iodriver.operations.Operation;

/**
 * Reports workload progress on the system console.
 */
public class ConsoleProgressReporter implements Closeable
{
    /**
     * Constructor.
     * 
     * @param output Stream to output to, usually {@code System.out}.
     */
    public ConsoleProgressReporter(PrintStream output,
                                   WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");

        _closed = new AtomicBoolean(false);
        _output = output;
        _operationExecutedToken = context.subscribeIfProvided(OperationExecuted.class,
                                                             this::onOperationExecuted);
        _volumeAddedToken = context.subscribeIfProvided(VolumeAdded.class,
                                                       this::onVolumeAdded);
        _volumeModifiedToken = context.subscribeIfProvided(VolumeModified.class,
                                                          this::onVolumeModified);
    }

    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            if (_operationExecutedToken.isPresent()) { _operationExecutedToken.get().close(); }
            if (_volumeAddedToken.isPresent()) { _volumeAddedToken.get().close(); }
            if (_volumeModifiedToken.isPresent()) { _volumeModifiedToken.get().close(); }
        }
    }

    protected void onOperationExecuted(Event<Operation> event)
    {
        if (event == null) throw new NullArgumentException("operationExecuted");
        
        _output.println("[" + event.getTimestamp() + "] Executing: " + event.getData());
    }
    
    protected void onVolumeAdded(Event<Volume> event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        Volume volume = event.getData();
        
        _output.println(
                "Adding volume ID " + volume.getId() + " " + javaString(volume.getName()));
    }
    
    protected void onVolumeModified(Event<BeforeAfter<Volume>> event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        BeforeAfter<Volume> volume = event.getData();
        Volume before = volume.getBefore();
        Volume after = volume.getAfter();
        
        _output.println(
                "Modifying volume ID " + before.getId() + " " + javaString(before.getName())
                + " -> " + after.getId() + " " + javaString(after.getName()));
    }
    
    /**
     * Whether this object has been closed.
     */
    private final AtomicBoolean _closed;

    /**
     * Operation executed event token.
     */
    private final Optional<Closeable> _operationExecutedToken;
    
    /**
     * Output.
     */
    private final PrintStream _output;
    
    private final Optional<Closeable> _volumeAddedToken;
    
    private final Optional<Closeable> _volumeModifiedToken;
}
