package com.formationds.iodriver.validators;

import static com.formationds.commons.util.Strings.javaString;

import java.text.NumberFormat;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.Validated;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.validators.RateLimitValidator.ValidationResult.VolumeResult;
import com.formationds.iodriver.workloads.QosWorkload;
import com.formationds.iodriver.workloads.QosWorkload.VolumeQosStats;
import com.google.common.collect.ImmutableList;

/**
 * Validate that no volume exceeded its throttle by more than 1%.
 */
public final class RateLimitValidator implements Validator
{
    public final static class ValidationResult extends com.formationds.iodriver.reporters.ValidationResult
    {
        public final static class VolumeResult extends com.formationds.iodriver.reporters.ValidationResult
        {
            public VolumeResult(boolean isValid,
                                String volumeName,
                                int throttle,
                                double iops)
            {
                super(isValid);
                
                if (volumeName == null) throw new NullArgumentException("volumeName");
                
                _iops = iops;
                _throttle = throttle;
                _volumeName = volumeName;
            }
            
            @Override
            public String toString()
            {
                NumberFormat iopsFormatter = NumberFormat.getNumberInstance();
                iopsFormatter.setMinimumFractionDigits(2);
                iopsFormatter.setMaximumFractionDigits(2);
                
                NumberFormat deviationFormatter = NumberFormat.getPercentInstance();
                deviationFormatter.setMinimumFractionDigits(2);
                deviationFormatter.setMaximumFractionDigits(2);
                
                return javaString(_volumeName) + ":"
                       + " Throttle: " + _throttle
                       + " IOPS: " + iopsFormatter.format(_iops)
                       + " Deviation: " + deviationFormatter.format((_iops - _throttle) / _throttle)
                       + " " + (isValid() ? "PASS"
                                          : "FAIL");
            }
            
            private final double _iops;
            
            private final int _throttle;
            
            private final String _volumeName;
        }
        
        public ValidationResult(boolean isValid,
                                Iterable<VolumeResult> volumeResults)
        {
            super(isValid);
            
            if (volumeResults == null) throw new NullArgumentException("volumeResults");
            
            _volumeResults = ImmutableList.copyOf(volumeResults);
        }
        
        @Override
        public String toString()
        {
            return "Rate limit validation: " + (isValid() ? "PASS"
                                                          : "FAIL") + "\n"
                   + _volumeResults.stream()
                                   .map(Objects::toString)
                                   .collect(Collectors.joining("\n"));
        }
        
        private final ImmutableList<VolumeResult> _volumeResults;
    }
    
    @Override
    public boolean isValid(WorkloadContext context)
    {
        if (context == null) throw new NullArgumentException("context");
        if (!(context instanceof QosWorkload.Context))
        {
            throw new IllegalArgumentException("context must come from newContext().");
        }
        
        QosWorkload.Context typedContext = (QosWorkload.Context)context;

        List<VolumeResult> volumeResults = new ArrayList<>();
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

                boolean isValid = Math.abs(deviation) <= 0.05;
                
                volumeResults.add(new VolumeResult(isValid, volumeName, throttle, iops));
                
                failed |= !isValid;
            }
        }

        context.sendIfRegistered(new Validated(Instant.now(),
                                               new ValidationResult(!failed, volumeResults)));
        
        return !failed;
    }
}
