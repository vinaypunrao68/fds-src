package com.formationds.iodriver.reporters;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.Validated;
import com.formationds.iodriver.events.VolumeModified;
import com.formationds.iodriver.workloads.QosWorkload.VolumeStarted;
import com.formationds.iodriver.workloads.QosWorkload.VolumeStopped;

public final class QosProgressReporter implements Closeable
{
    public QosProgressReporter(PrintStream output, WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");
        
        _closed = new AtomicBoolean(false);
        _output = output;
        _validatedToken = context.register(Validated.class, this::onValidated);
        _volumeModifiedToken = context.register(VolumeModified.class, this::onVolumeModified);
        _volumeStartToken = context.register(VolumeStarted.class, this::onVolumeStarted);
        _volumeStopToken = context.register(VolumeStopped.class, this::onVolumeStopped);
    }
    
    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _validatedToken.close();
            _volumeModifiedToken.close();
            _volumeStartToken.close();
            _volumeStopToken.close();
        }
    }
    
    protected void onValidated(Validated event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        _output.println(event.getData());
    }
    
    protected void onVolumeModified(VolumeModified event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        BeforeAfter<Volume> volume = event.getData();
        Volume currentVolume = volume.getAfter();
        QosPolicy qosPolicy = currentVolume.getQosPolicy();
        _output.println(javaString(currentVolume.getName()) + ":"
                        + " Assured: " + qosPolicy.getIopsMin()
                        + " Throttle: " + qosPolicy.getIopsMax());
    }
    
    protected void onVolumeStarted(VolumeStarted event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        _output.println(javaString(event.getData()) + ": Starting.");
    }
    
    protected void onVolumeStopped(VolumeStopped event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        _output.println(javaString(event.getData()) + ": Firished.");
    }
    
    private final AtomicBoolean _closed;
    
    private final PrintStream _output;
    
    private final Closeable _validatedToken;
    
    private final Closeable _volumeModifiedToken;
    
    private final Closeable _volumeStartToken;
    
    private final Closeable _volumeStopToken;
}
