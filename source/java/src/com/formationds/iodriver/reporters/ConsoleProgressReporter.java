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

public final class ConsoleProgressReporter implements Closeable
{
    public ConsoleProgressReporter(PrintStream output,
                                   Observable<Entry<String, Instant>> started,
                                   Observable<Entry<String, Instant>> stopped,
                                   Observable<Entry<String, VolumeQosSettings>> volumeAdded)
    {
        if (output == null) throw new NullArgumentException("output");
        if (started == null) throw new NullArgumentException("started");
        if (stopped == null) throw new NullArgumentException("stopped");
        if (volumeAdded == null) throw new NullArgumentException("volumeAdded");

        _closed = new AtomicBoolean(false);
        _output = output;
        _started = started.register(this::onStarted);
        _stopped = stopped.register(this::onStopped);
        _volumeAdded = volumeAdded.register(this::onVolumeAdded);
    }

    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _started.close();
            _stopped.close();
            _volumeAdded.close();
            
            _closed.set(true);
        }
    }

    private void onStarted(Entry<String, Instant> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        _output.println("Volume " + volume.getKey() + " started: " + volume.getValue());
    }
    
    private void onStopped(Entry<String, Instant> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        _output.println("Volume " + volume.getKey() + " stopped: " + volume.getValue());
    }
    
    private void onVolumeAdded(Entry<String, VolumeQosSettings> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        _output.println("Volume: " + volume.getKey() + " added: " + volume.getValue());
    }

    private final AtomicBoolean _closed;
    
    private final PrintStream _output;
    
    private final Closeable _started;

    private final Closeable _stopped;

    private final Closeable _volumeAdded;
}
