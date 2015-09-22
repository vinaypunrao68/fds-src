package com.formationds.iodriver.reporters;

import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.Operation;

/**
 * Captures events from a {@link com.formationds.iodriver.workloads.Workload Workload} run.
 * 
 * Gathers statistics from those events. Passes on events to multiple consumers.
 */
public final class WorkloadEventListener extends AbstractWorkloadEventListener
{
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
    
    public void close()
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

        VolumeQosStats stats = _getStatsInternal(volume);
        return stats.performance.getStart();
    }

    public VolumeQosStats getStats(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = _getStatsInternal(volume);

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

        VolumeQosStats stats = _getStatsInternal(volume);
        return stats.performance.getStop();
    }

    public Set<String> getVolumes()
    {
        return new HashSet<>(_volumeOps.keySet());
    }

    public void reportIo(String volume, int count)
    {
        if (volume == null) throw new NullArgumentException("volume");
        if (count <= 0) throw new IllegalArgumentException("count of IOs must be > 0.");

        VolumeQosStats stats = _getStatsInternal(volume);
        stats.performance.addOps(count);
    }

    @Override
    public void reportOperationExecution(Operation operation)
    {
        if (operation == null) throw new NullArgumentException("operation");
        
        operationExecuted.send(operation);
    }
    
    public void reportVolumeAdded(String name, VolumeQosSettings params)
    {
        if (name == null) throw new NullArgumentException("name");
        if (params == null) throw new NullArgumentException("params");

        if (_volumeOps.containsKey(name))
        {
            throw new IllegalArgumentException("Volume " + name + " already exists.");
        }

        _volumeOps.put(name, new VolumeQosStats(params));
    }
    
    public void reportVolumeModified(String name, VolumeQosSettings params)
    {
        if (name == null) throw new NullArgumentException("name");
        if (params == null) throw new NullArgumentException("params");
        
        if (!_volumeOps.containsKey(name))
        {
            throw new IllegalArgumentException("Volume " + name + " does not exist.");
        }
        
        _volumeOps.put(name, new VolumeQosStats(params));
    }

    public void reportVolumeStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = _getStatsInternal(volume);
        stats.performance.startNow();
    }
    
    public void reportVolumeStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = _getStatsInternal(volume);
        stats.performance.stopNow();
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
    private VolumeQosStats _getStatsInternal(String volume)
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
