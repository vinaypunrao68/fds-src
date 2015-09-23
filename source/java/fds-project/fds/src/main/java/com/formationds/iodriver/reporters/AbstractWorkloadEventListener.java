package com.formationds.iodriver.reporters;

import java.io.Closeable;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.operations.Operation;

public abstract class AbstractWorkloadEventListener implements Closeable
{
    /**
     * Notifies when the workload is finished running.
     */
    public final transient Subject<Void> finished;

    public final transient Subject<Operation> operationExecuted;
    
    public abstract AbstractWorkloadEventListener copy();
    
    /**
     * Get the default logger.
     *
     * @return Something to log to that will hopefully get a message to a user.
     */
    public Logger getLogger()
    {
        return _logger;
    }
    
    public abstract void reportOperationExecution(Operation operation);
    
    protected class CopyHelper
    {
        public final Logger logger = AbstractWorkloadEventListener.this._logger;
    }
    
    protected AbstractWorkloadEventListener(CopyHelper copyHelper)
    {
        if (copyHelper == null) throw new NullArgumentException("copyHelper");
        
        finished = new Subject<>();
        operationExecuted = new Subject<>();
        
        _logger = copyHelper.logger;
    }
    
    /**
     * Constructor.
     *
     * @param logger Someplace people can log stuff in an emergency.
     */
    protected AbstractWorkloadEventListener(Logger logger)
    {
        if (logger == null) throw new NullArgumentException("logger");
        
        finished = new Subject<>();
        operationExecuted = new Subject<>();
        
        _logger = logger;
    }
    
    /**
     * A last-resort place to report user-readable strings.
     */
    private final Logger _logger;
}
