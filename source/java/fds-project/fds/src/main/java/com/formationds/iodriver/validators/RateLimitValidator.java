package com.formationds.iodriver.validators;

import java.io.Closeable;
import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;

/**
 * Validate that no volume exceeded its throttle by more than 1%.
 */
public final class RateLimitValidator extends QosValidator
{
    @Override
    public boolean isValid(Closeable context)
    {
        if (context == null) throw new NullArgumentException("context");
        if (!(context instanceof Context))
        {
            throw new IllegalArgumentException("context must come from newContext().");
        }
        
        Context typedContext = (Context)context;

        boolean failed = false;
        for (String volumeName : typedContext.getVolumes())
        {
            VolumeQosStats stats = typedContext.getStats(volumeName);
            VolumeQosSettings params = stats.params;
            VolumeQosPerformance perf = stats.performance;

            if (perf.isStopped())
            {
                Instant start = perf.getStart();
                Instant stop = perf.getStop();
    
                int throttle = params.getIopsThrottle();
                Duration duration = Duration.between(start, stop);
                double durationInSeconds = duration.toMillis() / 1000.0;
                double iops = perf.getOps() / durationInSeconds;
                double deviation = (iops - throttle) / throttle;
    
                System.out.println(volumeName + ": (A:" + params.getIopsAssured() + ", T:"
                                   + params.getIopsThrottle() + "): " + perf.getOps() + " / "
                                   + durationInSeconds + " = " + iops + "(" + deviation * 100.0 + "%).");
    
                if (Math.abs(deviation) > 0.05)
                {
                    failed = true;
                }
            }
        }

        return !failed;
    }
}
