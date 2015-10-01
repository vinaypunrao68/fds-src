package com.formationds.iodriver.validators;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.text.NumberFormat;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.validators.AssuredRateValidator.ValidationResult.VolumeResult;
import com.google.common.collect.ImmutableList;

public final class AssuredRateValidator extends QosValidator
{
    public final static class ValidationResult extends WorkloadEventListener.ValidationResult
    {
        public final static class VolumeResult extends WorkloadEventListener.ValidationResult
        {
            public VolumeResult(boolean isValid, String volumeName, int assured, double iops)
            {
                super(isValid);
                
                if (volumeName == null) throw new NullArgumentException("volumeName");
                
                _assured = assured;
                _iops = iops;
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
                       + " Assured: " + _assured
                       + " IOPS: " + iopsFormatter.format(_iops)
                       + " Deviation: " + deviationFormatter.format((_iops - _assured) / _assured)
                       + " " + (isValid() ? "PASS" : "FAIL");
            }
            
            private final int _assured;
            
            private final double _iops;
            
            private final String _volumeName;
        }
        
        public ValidationResult(boolean isValid,
                                int targetIops,
                                double totalIops,
                                Iterable<VolumeResult> volumeResults)
        {
            super(isValid);
            
            if (volumeResults == null) throw new NullArgumentException("volumeResults");
            
            _targetIops = targetIops;
            _totalIops = totalIops;
            _volumeResults = ImmutableList.copyOf(volumeResults);
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
            
            return "Assured rate validation:"
                   + " Target IOPS: " + iopsFormatter.format(_targetIops)
                   + " Actual IOPS: " + iopsFormatter.format(_totalIops)
                   + " Deviation: " + deviationFormatter.format((_totalIops - _targetIops) / _targetIops)
                   + " " + (isValid() ? "PASS" : "FAIL") + "\n"
                   + _volumeResults.stream()
                                   .map(Objects::toString)
                                   .collect(Collectors.joining("\n"));
        }
        
        private final int _targetIops;
        
        private final double _totalIops;
        
        private final ImmutableList<VolumeResult> _volumeResults;
    }
    
    @Override
    public boolean isValid(Closeable context, WorkloadEventListener listener)
    {
        if (context == null) throw new NullArgumentException("context");
        if (listener == null) throw new NullArgumentException("listener");
        if (!(context instanceof Context))
        {
            throw new IllegalArgumentException("context must come from newContext().");
        }
        
        Context typedContext = (Context)context;

        List<VolumeResult> volumeResults = new ArrayList<>();
        boolean failed = false;
        int totalAssuredIops = 0;
        double totalIops = 0.0;
        for (String volumeName : typedContext.getVolumes())
        {
            VolumeQosStats stats = typedContext.getStats(volumeName);
            VolumeQosSettings params = stats.params;
            VolumeQosPerformance perf = stats.performance;
            
            if (perf.isStopped())
            {
                Instant start = perf.getStart();
                Instant stop = perf.getStop();

                int assured = params.getIopsAssured();
                Duration duration = Duration.between(start, stop);
                double durationInSeconds = duration.toMillis() / 1000.0;
                double iops = perf.getOps() / durationInSeconds;
                double deviation = (iops - assured) / assured;

                // We must get at least 95% of our assured rate to call it success. If we're more
                // than 50% over, there may have not been enough competition for this to be a good
                // test.
                boolean isValid = deviation >= -0.05 && deviation <= 0.5;
                
                totalAssuredIops += assured;
                totalIops += iops;
                volumeResults.add(new VolumeResult(isValid, volumeName, assured, iops));
                
                failed |= !isValid;
            }
        }
        
        // If we can go over 10% of what we requested for assured, we're not stressing the system
        // hard enough.
        if (Math.abs((totalIops - totalAssuredIops) / totalAssuredIops) > 0.1)
        {
            failed = true;
        }

        listener.validated.send(new ValidationResult(!failed, totalAssuredIops, totalIops, volumeResults));
        
        return !failed;
    }
}
