package com.formationds.iodriver.reporters;

import java.io.Closeable;
import java.util.HashMap;
import java.util.Map;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.Operation;

/**
 * Captures events from a {@link com.formationds.iodriver.workloads.Workload Workload} run.
 * 
 * Gathers statistics from those events. Passes on events to multiple consumers.
 */
public final class WorkloadEventListener implements Closeable
{
    public final static class BeforeAfter<T>
    {
        public BeforeAfter(T before, T after)
        {
            _before = before;
            _after = after;
        }
        
        public T getAfter()
        {
            return _after;
        }
        
        public T getBefore()
        {
            return _before;
        }
        
        private final T _after;
        
        private final T _before;
    }
    
    public final transient Subject<Object> adHocStart;
    
    public final transient Subject<Object> adHocStop;

    public final transient Subject<Operation> operationExecuted;
    
    public final transient Subject<Volume> volumeAdded;
    
    public final transient Subject<BeforeAfter<Volume>> volumeModified;
    
    public final transient Subject<Volume> volumeStatted;

    /**
     * Constructor.
     */
    public WorkloadEventListener(Logger logger)
    {
        this(new HashMap<>(), logger);
    }

    /**
     * Construct with parameters.
     * 
     * @param params QoS statistics.
     */
    public WorkloadEventListener(Map<String, VolumeQosSettings> params, Logger logger)
    {
        if (params == null) throw new NullArgumentException("params");
        if (logger == null) throw new NullArgumentException("logger");

        _logger = logger;
        
        adHocStart = new Subject<>();
        adHocStop = new Subject<>();
        operationExecuted = new Subject<>();
        volumeAdded = new Subject<>();
        volumeModified = new Subject<>();
        volumeStatted = new Subject<>();
    }
    
    public void close()
    {
        // Nothing for now.
    }
    
    public WorkloadEventListener copy()
    {
        return new WorkloadEventListener(new CopyHelper());
    }

    protected class CopyHelper
    {
        public final Logger logger = WorkloadEventListener.this._logger;
    }
    
    protected WorkloadEventListener(CopyHelper copyHelper)
    {
        if (copyHelper == null) throw new NullArgumentException("copyHelper");
        
        _logger = copyHelper.logger;
        
        adHocStart = new Subject<>();
        adHocStop = new Subject<>();
        operationExecuted = new Subject<>();
        volumeAdded = new Subject<>();
        volumeModified = new Subject<>();
        volumeStatted = new Subject<>();
    }
    
    private final Logger _logger;
}
