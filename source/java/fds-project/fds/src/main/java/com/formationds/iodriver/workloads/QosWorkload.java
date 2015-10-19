package com.formationds.iodriver.workloads;

import static com.formationds.commons.util.Strings.javaString;

import java.io.Closeable;
import java.io.IOException;
import java.io.PrintStream;
import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.patterns.Subject;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.WorkloadContext;
import com.formationds.iodriver.events.Event;
import com.formationds.iodriver.events.OperationExecuted;
import com.formationds.iodriver.events.Validated;
import com.formationds.iodriver.events.VolumeAdded;
import com.formationds.iodriver.events.VolumeModified;
import com.formationds.iodriver.events.VolumeStatted;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.categories.IoOperation;
import com.formationds.iodriver.reporters.BeforeAfter;
import com.formationds.iodriver.reporters.QosProgressReporter;

public abstract class QosWorkload extends Workload
{
    public final class Context extends WorkloadContext
    {
        public Context(Logger logger)
        {
            super(logger);
            
            _volumeOps = new HashMap<>();
            
            _operationExecutedToken = null;
            _volumeAddedToken = null;
            _volumeModifiedToken = null;
            _volumeStattedToken = null;
            _volumeStartToken = null;
            _volumeStopToken = null;
        }
        
        public VolumeQosStats getStats(String volumeName)
        {
            if (volumeName == null) throw new NullArgumentException("volumeName");
            
            VolumeQosStats stats = _getStatsInternal(volumeName);
            
            return stats.copy();
        }
        
        public Instant getVolumeStart(String volumeName)
        {
            if (volumeName == null) throw new NullArgumentException("volumeName");
            
            VolumeQosStats stats = _getStatsInternal(volumeName);
            return stats.performance.getStart();
        }
        
        public Instant getVolumeStop(String volumeName)
        {
            if (volumeName == null) throw new NullArgumentException("volumeName");
            
            VolumeQosStats stats = _getStatsInternal(volumeName);
            return stats.performance.getStop();
        }
        
        public Set<String> getVolumes()
        {
            return new HashSet<>(_volumeOps.keySet());
        }

        @Override
        protected void closeInternal() throws IOException
        {
            _operationExecutedToken.close();
            _volumeAddedToken.close();
            _volumeModifiedToken.close();
            _volumeStattedToken.close();
            _volumeStartToken.close();
            _volumeStopToken.close();
            
            super.closeInternal();
        }
        
        @Override
        protected void setUp()
        {
            super.setUp();
            
            Subject<OperationExecuted> operationExecuted = new Subject<>();
            Subject<Validated> validated = new Subject<>();
            Subject<VolumeAdded> volumeAdded = new Subject<>();
            Subject<VolumeModified> volumeModified = new Subject<>();
            Subject<VolumeStatted> volumeStatted = new Subject<>();
            Subject<VolumeStarted> volumeStarted = new Subject<>();
            Subject<VolumeStopped> volumeStopped = new Subject<>();
            
            _operationExecutedToken = operationExecuted.register(this::_onOperationExecuted);
            _volumeAddedToken = volumeAdded.register(this::_onVolumeAdded);
            _volumeModifiedToken = volumeModified.register(this::_onVolumeModified);
            _volumeStattedToken = volumeStatted.register(this::_onVolumeStatted);
            _volumeStartToken = volumeStarted.register(this::_onVolumeStart);
            _volumeStopToken = volumeStopped.register(this::_onVolumeStop);
            
            registerEvent(OperationExecuted.class, operationExecuted);
            registerEvent(Validated.class, validated);
            registerEvent(VolumeAdded.class, volumeAdded);
            registerEvent(VolumeModified.class, volumeModified);
            registerEvent(VolumeStatted.class, volumeStatted);
            registerEvent(VolumeStarted.class, volumeStarted);
            registerEvent(VolumeStopped.class, volumeStopped);
        }

        private Closeable _operationExecutedToken;
        
        private Closeable _volumeAddedToken;
        
        private Closeable _volumeModifiedToken;
        
        private Closeable _volumeStattedToken;
        
        private Closeable _volumeStartToken;
        
        private Closeable _volumeStopToken;
        
        /**
         * The statistics for volumes.
         */
        private final Map<String, VolumeQosStats> _volumeOps;
        
        private VolumeQosStats _getStatsInternal(String volumeName)
        {
            if (volumeName == null) throw new NullArgumentException("volumeName");
            
            VolumeQosStats stats = _volumeOps.get(volumeName);
            if (stats == null)
            {
                throw new IllegalArgumentException("No such volume: " + javaString(volumeName));
            }
            
            return stats;
        }
        
        private void _onOperationExecuted(OperationExecuted event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            Operation operation = event.getData();
            if (operation instanceof IoOperation)
            {
                IoOperation typedOperation = (IoOperation)operation;
                VolumeQosStats stats = _getStatsInternal(typedOperation.getVolumeName());
                VolumeQosPerformance performance = stats.performance;
                if (performance.isStarted() && !performance.isStopped())
                {
                    performance.addOps(typedOperation.getCost());
                }
            }
        }
        
        private void _onVolumeAdded(VolumeAdded event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            Volume volume = event.getData();
            String name = volume.getName();
            
            _volumeOps.put(name,  new VolumeQosStats(volume));
        }
        
        private void _onVolumeModified(VolumeModified event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            BeforeAfter<Volume> volume = event.getData();
            Volume before = volume.getBefore();
            Volume after = volume.getAfter();
            String oldName = before.getName();
            String newName = after.getName();
            VolumeQosStats stats = _getStatsInternal(oldName);
            if (!oldName.equals(newName))
            {
                _volumeOps.remove(oldName);
            }
            _volumeOps.put(newName, new VolumeQosStats(VolumeQosSettings.fromVolume(after),
                                                       stats.performance));
        }
        
        private void _onVolumeStart(VolumeStarted event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            VolumeQosStats stats = _volumeOps.get(event.getData());
            if (stats != null)
            {
                stats.performance.startNow();
            }
        }
        
        private void _onVolumeStatted(VolumeStatted event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            Volume volume = event.getData();
            String volumeName = volume.getName();
            VolumeQosStats stats = _volumeOps.get(volumeName);
            if (stats == null)
            {
                stats = new VolumeQosStats(VolumeQosSettings.fromVolume(volume));
            }
            else
            {
                stats = new VolumeQosStats(VolumeQosSettings.fromVolume(volume),
                                           stats.performance);
            }
            _volumeOps.put(volumeName, stats);
        }
        
        private void _onVolumeStop(VolumeStopped event)
        {
            if (event == null) throw new NullArgumentException("event");
            
            String volumeName = event.getData();
            VolumeQosStats stats = _volumeOps.get(volumeName);
            if (stats != null)
            {
                stats.performance.stopNow();
            }
        }
    }
    
    /**
     * QoS statistics used by this class.
     */
    public final static class VolumeQosStats
    {
        /**
         * QoS parameters.
         */
        public final VolumeQosSettings params;

        /**
         * Workload statistics.
         */
        public final VolumeQosPerformance performance;

        /**
         * Constructor.
         * 
         * @param params QoS parameters.
         */
        public VolumeQosStats(VolumeQosSettings params)
        {
            this(params, new VolumeQosPerformance());
        }
        
        public VolumeQosStats(Volume volume)
        {
            this(VolumeQosSettings.fromVolume(volume));
        }

        /**
         * Duplicate this object.
         * 
         * @return A deep copy of this object.
         */
        public VolumeQosStats copy()
        {
            return new VolumeQosStats(params.copy(), performance.copy());
        }

        /**
         * Constructor.
         * 
         * @param params QoS parameters.
         * @param performance Workload statistics.
         */
        private VolumeQosStats(VolumeQosSettings params, VolumeQosPerformance performance)
        {
            if (params == null) throw new NullArgumentException("params");
            if (performance == null) throw new NullArgumentException("performance");

            this.params = params;
            this.performance = performance;
        }
    }
    
    public static final class VolumeStarted extends _VolumeEvent
    {
        public VolumeStarted(Instant timestamp, String volumeName)
        {
            super(timestamp, volumeName);
        }
    }
    
    public static final class VolumeStopped extends _VolumeEvent
    {
        public VolumeStopped(Instant timestamp, String volumeName)
        {
            super(timestamp, volumeName);
        }
    }
    
    @Override
    public Optional<Closeable> getSuggestedReporter(PrintStream output,
                                                    WorkloadContext context)
    {
        if (output == null) throw new NullArgumentException("output");
        if (context == null) throw new NullArgumentException("context");
        
        return Optional.of(new QosProgressReporter(output, context));
    }

    @Override
    public Context newContext(Logger logger)
    {
        if (logger == null) throw new NullArgumentException("logger");
        
        return new Context(logger);
    }
    
    private static class _VolumeEvent extends Event<String>
    {
        public _VolumeEvent(Instant timestamp, String volumeName)
        {
            super(timestamp);
            
            if (volumeName == null) throw new NullArgumentException("volumeName");
            
            _volumeName = volumeName;
        }
        
        @Override
        public String getData()
        {
            return _volumeName;
        }
        
        private final String _volumeName;
    }
}
