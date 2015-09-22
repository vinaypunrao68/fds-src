package com.formationds.iodriver.reporters;

import java.io.Closeable;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.Operation;

public abstract class AbstractWorkloadEventListener implements Closeable
{
    /**
     * QoS statistics used by this class.
     */
    public final static class VolumeQosStats
    {
        /**
         * QoS parameters.
         */
        public final VolumeQosSettings params;

        /**
         * Workload statistics.
         */
        public final VolumeQosPerformance performance;

        /**
         * Constructor.
         * 
         * @param params QoS parameters.
         */
        public VolumeQosStats(VolumeQosSettings params)
        {
            this(params, new VolumeQosPerformance());
        }

        /**
         * Duplicate this object.
         * 
         * @return A deep copy of this object.
         */
        public VolumeQosStats copy()
        {
            return new VolumeQosStats(params.copy(), performance.copy());
        }

        /**
         * Constructor.
         * 
         * @param params QoS parameters.
         * @param performance Workload statistics.
         */
        private VolumeQosStats(VolumeQosSettings params, VolumeQosPerformance performance)
        {
            if (params == null) throw new NullArgumentException("params");
            if (performance == null) throw new NullArgumentException("performance");

            this.params = params;
            this.performance = performance;
        }
    }

    /**
     * Notifies when the workload is finished running.
     */
    public final Subject<Void> finished;

    public final Subject<Operation> operationExecuted;
    
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
