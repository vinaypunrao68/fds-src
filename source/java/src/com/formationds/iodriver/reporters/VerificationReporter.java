package com.formationds.iodriver.reporters;

import java.time.Duration;
import java.time.Instant;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Strings;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;

public final class VerificationReporter
{
    public final static class VolumeQosStats
    {
        public final VolumeQosSettings params;
        public final VolumeQosPerformance performance;

        public VolumeQosStats(VolumeQosSettings params)
        {
            if (params == null) throw new NullArgumentException("params");

            this.params = params;
            this.performance = new VolumeQosPerformance();
        }
    }

    public VerificationReporter()
    {
        this(new HashMap<>());
    }

    public VerificationReporter(Map<String, VolumeQosSettings> params)
    {
        if (params == null) throw new NullArgumentException("params");

        _volumeOps = new HashMap<>();
        for (Entry<String, VolumeQosSettings> param : params.entrySet())
        {
            String volumeName = param.getKey();
            VolumeQosSettings value = param.getValue();
            if (value == null)
            {
                throw new IllegalArgumentException("null entry for params["
                                                   + Strings.javaString(volumeName) + "].");
            }
            _volumeOps.put(volumeName, new VolumeQosStats(value));
        }
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
    }

    public Instant getStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStats(volume);
        return stats.performance.getStart();
    }

    public Instant getStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStats(volume);
        return stats.performance.getStop();
    }

    public void reportIo(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStats(volume);
        stats.performance.addOps(1);
    }

    public void reportStart(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStats(volume);
        stats.performance.startNow();
    }

    public void reportStop(String volume)
    {
        if (volume == null) throw new NullArgumentException("volume");

        VolumeQosStats stats = getStats(volume);
        stats.performance.stopNow();
    }

    public boolean wereIopsInRange()
    {
        for (Entry<String, VolumeQosStats> entry : _volumeOps.entrySet())
        {
            String volumeName = entry.getKey();
            VolumeQosStats stats = entry.getValue();

            Instant start = stats.performance.getStart();
            Instant stop = stats.performance.getStop();
            if (start == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been started.");
            }
            if (stop == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been stopped.");
            }

            Duration duration = Duration.between(start, stop);
            double durationInSeconds = duration.toMillis() / 1000.0;
            double iops = stats.performance.getOps() / durationInSeconds;

            System.out.println(volumeName + ": " + stats.performance.getOps() + " / "
                               + durationInSeconds + " = " + iops + ".");

            if (iops < stats.params.getIopsAssured() || iops > stats.params.getIopsThrottle())
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
