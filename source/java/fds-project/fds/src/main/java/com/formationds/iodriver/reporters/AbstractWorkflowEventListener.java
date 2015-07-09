package com.formationds.iodriver.reporters;

import java.time.Instant;
import java.util.Map.Entry;
import java.util.Set;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;

public abstract class AbstractWorkflowEventListener
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

    /**
     * Notifies when the workload has started the main body.
     */
    public final Subject<Entry<String, Instant>> started;

    /**
     * Notifies when the workload has stopped the main body.
     */
    public final Subject<Entry<String, Instant>> stopped;

    /**
     * Notifies when the workload has added a volume.
     */
    public final Subject<Entry<String, VolumeQosSettings>> volumeAdded;

    /**
     * Call when the workload adds a volume.
     * 
     * @param name The volume name.
     * @param params The volume's QoS parameters.
     */
    public abstract void addVolume(String name, VolumeQosSettings params);
    
    /**
     * Call when the workload is completely done.
     */
    public abstract void finished();

    /**
     * Get the default logger.
     *
     * @return Something to log to that will hopefully get a message to a user.
     */
    public Logger getLogger()
    {
        return _logger;
    }
    
    /**
     * Get the statistics for a given volume.
     * 
     * @param volume The name of the volume to look up.
     * 
     * @return The current statistics for the volume. These may not be complete if the workload has
     *         not finished.
     */
    public abstract VolumeQosStats getStats(String volume);

    /**
     * Get the names of all volumes that have been reported to this listener.
     * 
     * @return A set of unique volume names.
     */
    public abstract Set<String> getVolumes();

    /**
     * Call when an I/O operation occurs and the number of I/Os must be specified.
     * 
     * @param volume The volume that received the I/O.
     * @param count The number of I/Os sent.
     */
    public abstract void reportIo(String volume, int count);
    
    /**
     * Call when the main body of a workload is starting. Must not be called more than once for a
     * given volume.
     * 
     * @param volume The volume name.
     */
    public abstract void reportStart(String volume);
    
    /**
     * Call when the main body of a workload is finishing. Must not be called prior to
     * {@link #reportStart(String)} for {@code volume}, and must not be called more than once for a
     * given volume.
     * 
     * @param volume The volume name.
     */
    public abstract void reportStop(String volume);
    
    /**
     * Constructor.
     *
     * @param logger Someplace people can log stuff in an emergency.
     */
    protected AbstractWorkflowEventListener(Logger logger)
    {
        if (logger == null) throw new NullArgumentException("logger");
        
        finished = new Subject<>();
        started = new Subject<>();
        stopped = new Subject<>();
        volumeAdded = new Subject<>();
        
        _logger = logger;
    }
    
    /**
     * A last-resort place to report user-readable strings.
     */
    private final Logger _logger;
}
