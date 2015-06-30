package com.formationds.iodriver.reporters;

import java.util.Set;

import com.formationds.commons.NullArgumentException;
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
}
