package com.formationds.iodriver.reporters;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.WorkloadEventListener.BeforeAfter;

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
                                   WorkloadEventListener listener)
    {
        if (output == null) throw new NullArgumentException("output");
        if (listener == null) throw new NullArgumentException("listener");

        _closed = new AtomicBoolean(false);
        _output = output;
        _adHocStartToken = listener.adHocStart.register(this::onAdHocStart);
        _adHocStopToken = listener.adHocStop.register(this::onAdHocStop);
        _operationExecutedToken = listener.operationExecuted.register(this::onOperationExecuted);
        _volumeAddedToken = listener.volumeAdded.register(this::onVolumeAdded);
        _volumeModifiedToken = listener.volumeModified.register(this::onVolumeModified);
    }

    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _adHocStartToken.close();
            _adHocStopToken.close();
            _operationExecutedToken.close();
            _volumeAddedToken.close();
            _volumeModifiedToken.close();
        }
    }

    protected void onAdHocStart(Object event)
    {
        if (event != null)
        {
            _output.println("Ad-hoc start: " + event.toString());
        }
    }
    
    protected void onAdHocStop(Object event)
    {
        if (event != null)
        {
            _output.println("Ad-hoc stop: " + event.toString());
        }
    }
    
    protected void onOperationExecuted(Operation operation)
    {
        if (operation == null) throw new NullArgumentException("operationExecuted");
        
        _output.println("Executing: " + operation);
    }
    
    protected void onVolumeAdded(Volume volume)
    {
        if (volume != null)
        {
            _output.println(
                    "Adding volume ID " + volume.getId() + " " + javaString(volume.getName()));
        }
    }
    
    protected void onVolumeModified(BeforeAfter<Volume> volume)
    {
        if (volume != null)
        {
            Volume before = volume.getBefore();
            Volume after = volume.getAfter();
            
            _output.println(
                    "Modifying volume ID " + before.getId() + " " + javaString(before.getName())
                    + " -> " + after.getId() + " " + javaString(after.getName()));
        }
    }
    
    private final Closeable _adHocStartToken;
    
    private final Closeable _adHocStopToken;
    
    /**
     * Whether this object has been closed.
     */
    private final AtomicBoolean _closed;

    /**
     * Operation executed event token.
     */
    private final Closeable _operationExecutedToken;
    
    /**
     * Output.
     */
    private final PrintStream _output;
    
    private final Closeable _volumeAddedToken;
    
    private final Closeable _volumeModifiedToken;
}
