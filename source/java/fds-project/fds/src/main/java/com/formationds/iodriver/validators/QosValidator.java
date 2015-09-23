package com.formationds.iodriver.validators;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.reporters.AbstractWorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener.VolumeQosStats;

// TODO: Intended to verify that volumes get their assured IOPS without exceeding their throttles.
//       Kind of works, not complete.
public final class QosValidator implements Validator
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

            System.out.println(volumeName + "(A:" + stats.params.getIopsAssured() + ", T:"
                               + stats.params.getIopsThrottle() + ", P:"
                               + stats.params.getPriority() + ")" + ": "
                               + stats.performance.getOps() + " / " + durationInSeconds + " = "
                               + iops + ".");

            if (iops < stats.params.getIopsAssured() || iops > stats.params.getIopsThrottle())
            {
                failed = true;
            }
        }

        return !failed;
    }
}
