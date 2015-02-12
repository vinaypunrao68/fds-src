package com.formationds.iodriver.reporters;

import java.time.Duration;
import java.time.Instant;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.WorkflowEventListener.VolumeQosStats;

public final class RateLimitValidator implements Validator
{
    @Override
    public boolean isValid(WorkflowEventListener listener)
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
            double deviation = (iops - throttle) / throttle * 100.0;

            System.out.println(volumeName + ": A:" + params.getIopsAssured() + ", T:"
                               + params.getIopsThrottle() + "): " + perf.getOps() + " / "
                               + durationInSeconds + " = " + iops + "(" + deviation + ").");

            if (Math.abs(deviation) > 1)
            {
                failed = true;
            }
        }

        return !failed;
    }

}
