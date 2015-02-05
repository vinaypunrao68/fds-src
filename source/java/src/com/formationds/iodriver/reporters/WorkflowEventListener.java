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

public final class WorkflowEventListener
{
    public final static class VolumeQosStats
    {
        public final VolumeQosSettings params;
        public final VolumeQosPerformance performance;

        public VolumeQosStats(VolumeQosSettings params)
        {
            this(params, new VolumeQosPerformance());
        }

        public VolumeQosStats copy()
        {
            return new VolumeQosStats(params.copy(), performance.copy());
        }

        private VolumeQosStats(VolumeQosSettings params, VolumeQosPerformance performance)
        {
            if (params == null) throw new NullArgumentException("params");
            if (performance == null) throw new NullArgumentException("performance");

            this.params = params;
            this.performance = performance;
        }
    }

    public final Subject<Void> finished;
    public final Subject<Entry<String, Instant>> started;
    public final Subject<Entry<String, Instant>> stopped;
    public final Subject<Entry<String, VolumeQosSettings>> volumeAdded;

    public WorkflowEventListener()
    {
        this(new HashMap<>());
    }

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
    
    public Instant getStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        return stats.performance.getStart();
    }

    public VolumeQosStats getStats(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);

        return stats.copy();
    }

    public Instant getStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        return stats.performance.getStop();
    }

    public Set<String> getVolumes()
    {
        return new HashSet<>(_volumeOps.keySet());
    }

    public void reportIo(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        stats.performance.addOps(1);
    }

    public void reportStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        Instant start = stats.performance.startNow();
        
        started.send(new SimpleEntry<>(volume, start));
    }

    public void reportStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStatsInternal(volume);
        Instant stop = stats.performance.stopNow();
        
        stopped.send(new SimpleEntry<>(volume, stop));
    }

    private final Map<String, VolumeQosStats> _volumeOps;

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
