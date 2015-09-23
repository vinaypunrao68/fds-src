package com.formationds.iodriver.validators;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener.VolumeQosStats;

public class AssuredRateValidator implements Validator
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
        int totalAssuredIops = 0;
        double totalIops = 0.0;
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
            
            int assured = params.getIopsAssured();
            Duration duration = Duration.between(start, stop);
            double durationInSeconds = duration.toMillis() / 1000.0;
            double iops = perf.getOps() / durationInSeconds;
            double deviation = (iops - assured) / assured;

            totalAssuredIops += assured;
            totalIops += iops;

            System.out.println(volumeName + ": A:" + params.getIopsAssured() + ", T:"
                               + params.getIopsThrottle() + "): " + perf.getOps() + " / "
                               + durationInSeconds + " = " + iops + "(" + deviation * 100.0 + "%).");
            
            // We must be within at least 3% of our assured rate to call it success. If we're more
            // than 50% over, we're probably not hitting QoS limits hard enough.
            if (deviation < -0.05 || deviation > 0.50)
            {
                failed = true;
            }
        }
        
        double totalDeviation = (totalIops - totalAssuredIops) / totalAssuredIops;
        System.out.println("Total system IOPS: " + totalIops
                           + "(" + totalDeviation * 100.0 + "%).");

        // If we can go over 10% of what we requested for assured, we're not stressing the system
        // hard enough.
        if (Math.abs(totalDeviation) > 0.1)
        {
            failed = true;
        }

        return !failed;
    }
}
