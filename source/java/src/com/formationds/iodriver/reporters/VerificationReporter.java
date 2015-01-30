package com.formationds.iodriver.reporters;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;

public final class VerificationReporter
{
    public final static class VolumeQosParams
    {
        public final int assured_rate;
        public final int priority;
        public final int throttle_rate;

        public VolumeQosParams(int assured_rate, int throttle_rate, int priority)
        {
            this.assured_rate = assured_rate;
            this.throttle_rate = throttle_rate;
            this.priority = priority;
        }
    }

    public final static class VolumePerformanceData
    {
        public int ops;
        public Date start;
        public Date stop;
        
        public VolumePerformanceData()
        {
            ops = 0;
            start = null;
            stop = null;
        }
    }

    public final static class VolumeQosStats
    {
        public final VolumeQosParams params;
        public final VolumePerformanceData performance;

        public VolumeQosStats(VolumeQosParams params)
        {
            if (params == null) throw new NullArgumentException("params");

            this.params = params;
            this.performance = new VolumePerformanceData();
        }
    }

    public VerificationReporter()
    {
        this(new HashMap<>());
    }
    
    public VerificationReporter(Map<String, VolumeQosParams> params)
    {
        if (params == null) throw new NullArgumentException("params");

        _volumeOps = new HashMap<>();
        for (Entry<String, VolumeQosParams> param : params.entrySet())
        {
            String volumeName = param.getKey();
            VolumeQosParams value = param.getValue();
            if (value == null)
            {
                throw new IllegalArgumentException("null entry for params["
                                                   + Strings.javaString(volumeName) + "].");
            }
            _volumeOps.put(volumeName, new VolumeQosStats(value));
        }
    }
    
    public void addVolume(String name, VolumeQosParams params)
    {
        if (name == null) throw new NullArgumentException("name");
        if (params == null) throw new NullArgumentException("params");
        
        if (_volumeOps.containsKey(name))
        {
            throw new IllegalArgumentException("Volume " + name + " already exists.");
        }
        
        _volumeOps.put(name, new VolumeQosStats(params));
    }

    public void reportIo(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        VolumeQosStats stats = getStats(volume);
        ++stats.performance.ops;
    }
    
    public void reportStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        VolumeQosStats stats = getStats(volume);
        stats.performance.start = new Date();
    }
    
    public void reportStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        VolumeQosStats stats = getStats(volume);
        stats.performance.stop = new Date();
    }
    
    public boolean wereIopsInRange()
    {
        for (Entry<String, VolumeQosStats> entry: _volumeOps.entrySet())
        {
            String volumeName = entry.getKey();
            VolumeQosStats stats = entry.getValue();
            
            Date start = stats.performance.start;
            Date stop = stats.performance.stop;
            if (start == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been started.");
            }
            if (stop == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been stopped.");
            }
            
            long durationInMs = stop.getTime() - start.getTime();
            double durationInSeconds = durationInMs / 1000.0;
            double iops = stats.performance.ops / durationInSeconds;

            if (iops < stats.params.assured_rate || iops > stats.params.throttle_rate)
            {
                return false;
            }
        }

        return true;
    }

    private VolumeQosStats getStats(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        VolumeQosStats stats = _volumeOps.get(volume);
        if (stats == null)
        {
            throw new IllegalArgumentException("No such volume: " + volume);
        }
        
        return stats;
    }

    private final Map<String, VolumeQosStats> _volumeOps;
}
