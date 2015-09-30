package com.formationds.iodriver.reporters;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.util.concurrent.atomic.AtomicBoolean;

import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.reporters.WorkloadEventListener.BeforeAfter;
import com.formationds.iodriver.reporters.WorkloadEventListener.ValidationResult;

public final class QosProgressReporter implements Closeable
{
    public QosProgressReporter(PrintStream output, WorkloadEventListener listener)
    {
        if (output == null) throw new NullArgumentException("output");
        if (listener == null) throw new NullArgumentException("listener");
        
        _closed = new AtomicBoolean(false);
        _output = output;
        _adHocStartToken = listener.adHocStart.register(this::onAdHocStart);
        _adHocStopToken = listener.adHocStop.register(this::onAdHocStop);
        _validatedToken = listener.validated.register(this::onValidated);
        _volumeModifiedToken = listener.volumeModified.register(this::onVolumeModified);
    }
    
    @Override
    public void close() throws IOException
    {
        if (_closed.compareAndSet(false, true))
        {
            _adHocStartToken.close();
            _adHocStopToken.close();
            _validatedToken.close();
            _volumeModifiedToken.close();
        }
    }
    
    protected void onAdHocStart(Object event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        if (event instanceof String)
        {
            _output.println(javaString((String)event) + ": Starting.");
        }
    }
    
    protected void onAdHocStop(Object event)
    {
        if (event == null) throw new NullArgumentException("event");
        
        if (event instanceof String)
        {
            _output.println(javaString((String)event) + ": Finished.");
        }
    }
    
    protected void onValidated(ValidationResult result)
    {
        if (result == null) throw new NullArgumentException("result");
        
        _output.println(result);
    }
    
    protected void onVolumeModified(BeforeAfter<Volume> volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        Volume currentVolume = volume.getAfter();
        QosPolicy qosPolicy = currentVolume.getQosPolicy();
        _output.println(javaString(currentVolume.getName()) + ":"
                        + " Assured: " + qosPolicy.getIopsMin()
                        + " Throttle: " + qosPolicy.getIopsMax());
    }
    
    private final Closeable _adHocStartToken;
    
    private final Closeable _adHocStopToken;
    
    private final AtomicBoolean _closed;
    
    private final PrintStream _output;
    
    private final Closeable _validatedToken;
    
    private final Closeable _volumeModifiedToken;
}
