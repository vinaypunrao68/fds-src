package com.formationds.iodriver.reporters;

import java.time.Instant;
import java.util.AbstractMap.SimpleEntry;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.model.VolumeQosSettings;

/**
 * Captures events from a {@link com.formationds.iodriver.workloads.Workload Workload} run.
 * 
 * Gathers statistics from those events. Passes on events to multiple consumers.
 */
public final class WorkflowEventListener extends AbstractWorkflowEventListener
{
    /**
     * Constructor.
     */
    public WorkflowEventListener(Logger logger)
    {
        this(new HashMap<>(), logger);
    }

    /**
     * Construct with parameters.
     * 
     * @param params QoS statistics.
     */
    public WorkflowEventListener(Map<String, VolumeQosSettings> params, Logger logger)
    {
        super(logger);
        
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
    }

    @Override
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

    @Override
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

    @Override
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

        reportIo(volume, 1);
    }

    @Override
    public void reportIo(String volume, int count)
    {
        if (volume == null) throw new NullArgumentException("volume");
        if (count <= 0) throw new IllegalArgumentException("count of IOs must be > 0.");

        VolumeQosStats stats = getStatsInternal(volume);
        stats.performance.addOps(count);
    }

    @Override
    public void reportStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        Instant start = stats.performance.startNow();

        started.send(new SimpleEntry<>(volume, start));
    }

    @Override
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
