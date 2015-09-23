package com.formationds.iodriver.validators;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener.VolumeQosStats;

/**
 * Validate that no volume exceeded its throttle by more than 1%.
 */
public final class RateLimitValidator implements Validator
{
    @Override
    public boolean isValid(AbstractWorkloadEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");

        if (listener instanceof WorkloadEventListener)
        {
            return isValid((WorkloadEventListener)listener);
        }
        else
        {
            return false;
        }
    }
    
    public boolean isValid(WorkloadEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");
        
        boolean failed = false;
        for (String volumeName : listener.getVolumes())
        {
            VolumeQosStats stats = listener.getStats(volumeName);
            VolumeQosSettings params = stats.params;
            VolumeQosPerformance perf = stats.performance;

            Instant start = perf.getStart();
            Instant stop = perf.getStop();
            if (start == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been started.");
            }
            if (stop == null)
            {
                throw new IllegalStateException("Volume " + volumeName + " has not been stopped.");
            }

            int throttle = params.getIopsThrottle();
            Duration duration = Duration.between(start, stop);
            double durationInSeconds = duration.toMillis() / 1000.0;
            double iops = perf.getOps() / durationInSeconds;
            double deviation = (iops - throttle) / throttle;

            System.out.println(volumeName + ": A:" + params.getIopsAssured() + ", T:"
                               + params.getIopsThrottle() + "): " + perf.getOps() + " / "
                               + durationInSeconds + " = " + iops + "(" + deviation * 100.0 + "%).");

            if (Math.abs(deviation) > 0.05)
            {
                failed = true;
            }
        }

        return !failed;
    }
}
