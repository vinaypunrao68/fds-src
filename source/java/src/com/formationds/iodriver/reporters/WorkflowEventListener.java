package com.formationds.iodriver.reporters;

import java.time.Instant;
import java.util.AbstractMap.SimpleEntry;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.Strings;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;

/**
 * Captures events from a {@link com.formationds.iodriver.workloads.Workload Workload} run.
 * 
 * Gathers statistics from those events. Passes on events to multiple consumers.
 */
public final class WorkflowEventListener
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
     * Constructor.
     */
    public WorkflowEventListener()
    {
        this(new HashMap<>());
    }

    /**
     * Construct with parameters.
     * 
     * @param params QoS statistics.
     */
    public WorkflowEventListener(Map<String, VolumeQosSettings> params)
    {
        if (params == null) throw new NullArgumentException("params");

        _volumeOps = new HashMap<>();
        for (Entry<String, VolumeQosSettings> param : params.entrySet())
        {
            String volumeName = param.getKey();
            if (volumeName == null)
            {
                throw new IllegalArgumentException("null key found in params.");
            }
            VolumeQosSettings value = param.getValue();
            if (value == null)
            {
                throw new IllegalArgumentException("null value for params["
                                                   + Strings.javaString(volumeName) + "].");
            }
            _volumeOps.put(volumeName, new VolumeQosStats(value));
        }

        finished = new Subject<>();
        started = new Subject<>();
        stopped = new Subject<>();
        volumeAdded = new Subject<>();
    }

    /**
     * Call when the workload adds a volume.
     * 
     * @param name The volume name.
     * @param params The volume's QoS parameters.
     */
    public void addVolume(String name, VolumeQosSettings params)
    {
        if (name == null) throw new NullArgumentException("name");
        if (params == null) throw new NullArgumentException("params");

        if (_volumeOps.containsKey(name))
        {
            throw new IllegalArgumentException("Volume " + name + " already exists.");
        }

        _volumeOps.put(name, new VolumeQosStats(params));

        volumeAdded.send(new SimpleEntry<>(name, params));
    }

    /**
     * Call when the workload is completely done.
     */
    public void finished()
    {
        for (VolumeQosStats volumeStats : _volumeOps.values())
        {
            if (!volumeStats.performance.isStopped())
            {
                throw new IllegalStateException("Not all operations have finished!");
            }
        }

    }

    /**
     * Get the instant the workload started its main body for a given volume. This method must not
     * be called prior to the workload starting for that volume.
     * 
     * @param volume The volume name.
     * 
     * @return The instant the main body began executing for {@code volume}.
     */
    public Instant getStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        return stats.performance.getStart();
    }

    /**
     * Get the statistics for a given volume.
     * 
     * @param volume The name of the volume to look up.
     * 
     * @return The current statistics for the volume. These may not be complete if the workload has
     *         not finished.
     */
    public VolumeQosStats getStats(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);

        return stats.copy();
    }

    /**
     * Get the instant the workload stopped its main body for a given volume. This method must not
     * be called prior to the workload stopping for that volume.
     * 
     * @param volume The volume name.
     * 
     * @return The instant the main body finished executing for {@code volume}.
     */
    public Instant getStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        return stats.performance.getStop();
    }

    /**
     * Get the names of all volumes that have been reported to this listener.
     * 
     * @return A set of unique volume names.
     */
    public Set<String> getVolumes()
    {
        return new HashSet<>(_volumeOps.keySet());
    }

    /**
     * Call when an I/O operation occurs.
     * 
     * @param volume The volume that received I/O.
     */
    public void reportIo(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        stats.performance.addOps(1);
    }

    /**
     * Call when the main body of a workload is starting. Must not be called more than once for a
     * given volume.
     * 
     * @param volume The volume name.
     */
    public void reportStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        Instant start = stats.performance.startNow();

        started.send(new SimpleEntry<>(volume, start));
    }

    /**
     * Call when the main body of a workload is finishing. Must not be called prior to
     * {@link #reportStart(String)} for {@code volume}, and must not be called more than once for a
     * given volume.
     * 
     * @param volume The volume name.
     */
    public void reportStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        Instant stop = stats.performance.stopNow();

        stopped.send(new SimpleEntry<>(volume, stop));
    }

    /**
     * The statistics for volumes.
     */
    private final Map<String, VolumeQosStats> _volumeOps;

    /**
     * Get the statistics for a given volume. Must not be called for a volume that has not been
     * added via {@link #addVolume(String, VolumeQosSettings)}.
     * 
     * @param volume The volume name.
     * 
     * @return The volume's statistics.
     */
    private VolumeQosStats getStatsInternal(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = _volumeOps.get(volume);
        if (stats == null)
        {
            throw new IllegalArgumentException("No such volume: " + volume);
        }

        return stats;
    }
}
