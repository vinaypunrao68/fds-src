package com.formationds.iodriver.reporters;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.time.Instant;
import java.util.Map.Entry;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Observable;
import com.formationds.iodriver.model.VolumeQosSettings;
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
     * @param started An event source for workload start.
     * @param stopped An event source for workload stop.
     * @param volumeAdded An event source for volume adds.
     */
    public ConsoleProgressReporter(PrintStream output,
                                   Observable<Operation> operationExecuted,
                                   Observable<Entry<String, Instant>> started,
                                   Observable<Entry<String, Instant>> stopped,
                                   Observable<Entry<String, VolumeQosSettings>> volumeAdded)
    {
        if (output == null) throw new NullArgumentException("output");
        if (operationExecuted == null) throw new NullArgumentException("operationExecuted");
        if (started == null) throw new NullArgumentException("started");
        if (stopped == null) throw new NullArgumentException("stopped");
        if (volumeAdded == null) throw new NullArgumentException("volumeAdded");

        _closed = new AtomicBoolean(false);
        _output = output;
        _operationExecuted = operationExecuted.register(this::onOperationExecuted);
        _started = started.register(this::onStarted);
        _stopped = stopped.register(this::onStopped);
        _volumeAdded = volumeAdded.register(this::onVolumeAdded);
    }

    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _operationExecuted.close();
            _started.close();
            _stopped.close();
            _volumeAdded.close();

            _closed.set(true);
        }
    }

    private void onOperationExecuted(Operation operation)
    {
        if (operation == null) throw new NullArgumentException("operationExecuted");
        
        _output.println("Executing: " + operation);
    }
    
    /**
     * Handle started event.
     * 
     * @param volume The volume name and time of start.
     */
    private void onStarted(Entry<String, Instant> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        _output.println("Volume " + volume.getKey() + " started: " + volume.getValue());
    }

    /**
     * Handle stopped event.
     * 
     * @param volume The volume name and time of stop.
     */
    private void onStopped(Entry<String, Instant> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        _output.println("Volume " + volume.getKey() + " stopped: " + volume.getValue());
    }

    /**
     * Handle volume added event.
     * 
     * @param volume The volume name and QoS settings.
     */
    private void onVolumeAdded(Entry<String, VolumeQosSettings> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        _output.println("Volume: " + volume.getKey() + " added: " + volume.getValue());
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

    /**
     * Started event token.
     */
    private final Closeable _started;

    /**
     * Stopped event token.
     */
    private final Closeable _stopped;

    /**
     * Volume added event token.
     */
    private final Closeable _volumeAdded;
}
