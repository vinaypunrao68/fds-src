package com.formationds.iodriver.validators;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener;
import com.formationds.iodriver.reporters.AbstractWorkflowEventListener.VolumeQosStats;

public class AssuredRateValidator implements Validator
{
    @Override
    public boolean isValid(AbstractWorkflowEventListener listener)
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
            
            int assured = params.getIopsAssured();
            Duration duration = Duration.between(start, stop);
            double durationInSeconds = duration.toMillis() / 1000.0;
            double iops = perf.getOps() / durationInSeconds;
            double deviation = (iops - assured) / assured;
            
            System.out.println(volumeName + ": A:" + params.getIopsAssured() + ", T:"
                               + params.getIopsThrottle() + "): " + perf.getOps() + " / "
                               + durationInSeconds + " = " + iops + "(" + deviation * 100.0 + "%).");
            
            // We must be within at least 3% of our assured rate to call it success. If we're more
            // than 25% over, we're probably not hitting QoS limits hard enough.
            if (deviation < -0.03 || deviation > 0.25)
            {
                failed = true;
            }
        }
        
        return !failed;
    }
}
