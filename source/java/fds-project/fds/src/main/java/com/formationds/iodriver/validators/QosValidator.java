package com.formationds.iodriver.validators;

import java.io.Closeable;
import java.io.IOException;
import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.model.VolumeQosPerformance;
import com.formationds.iodriver.model.VolumeQosSettings;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.operations.categories.IoOperation;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.formationds.iodriver.reporters.WorkloadEventListener.BeforeAfter;

public abstract class QosValidator implements Validator
{
    public final class Context implements Closeable
    {
        public Context(WorkloadEventListener listener)
        {
            if (listener == null) throw new NullArgumentException("listener");
            
            _volumeOps = new HashMap<>();

            // We're ready for events now.
            _operationExecutedToken = listener.operationExecuted.register(this::_onOperationExecuted);
            _volumeAddedToken = listener.volumeAdded.register(this::_onVolumeAdded);
            _volumeModifiedToken = listener.volumeModified.register(this::_onVolumeModified);
            _volumeStattedToken = listener.volumeStatted.register(this::_onVolumeStatted);
            _volumeStartToken = listener.adHocStart.register(this::_onVolumeStart);
            _volumeStopToken = listener.adHocStop.register(this::_onVolumeStop);
        }
        
        @Override
        public void close() throws IOException
        {
            for (VolumeQosStats volumeStats : _volumeOps.values())
            {
                if (!volumeStats.performance.isStopped())
                {
                    throw new IllegalStateException("Not all operations have finished!");
                }
            }
            
            _operationExecutedToken.close();
            _volumeAddedToken.close();
            _volumeModifiedToken.close();
            _volumeStattedToken.close();
            _volumeStartToken.close();
            _volumeStopToken.close();
        }
        
        public VolumeQosStats getStats(String volume)
        {
            if (volume == null) throw new NullArgumentException("volume");

            VolumeQosStats stats = _getStatsInternal(volume);

            return stats.copy();
        }

        public Instant getVolumeStart(String volume)
        {
            if (volume == null) throw new NullArgumentException("volume");
            
            VolumeQosStats stats = _getStatsInternal(volume);
            return stats.performance.getStart();
        }
        
        public Instant getVolumeStop(String volume)
        {
            if (volume == null) throw new NullArgumentException("volume");
            
            VolumeQosStats stats = _getStatsInternal(volume);
            return stats.performance.getStop();
        }
        
        public Set<String> getVolumes()
        {
            return new HashSet<>(_volumeOps.keySet());
        }

        private final Closeable _operationExecutedToken;
        
        private final Closeable _volumeAddedToken;
        
        private final Closeable _volumeModifiedToken;
        
        private final Closeable _volumeStartToken;
        
        private final Closeable _volumeStattedToken;
        
        private final Closeable _volumeStopToken;
        
        /**
         * The statistics for volumes.
         */
        private final Map<String, VolumeQosStats> _volumeOps;

        /**
         * Get the statistics for a given volume. Must not be called for a volume that has not been
         * added via {@link #addVolume(String, VolumeQosSettings)}.
         * 
         * @param volume The volume name.
         * 
         * @return The volume's statistics.
         */
        private VolumeQosStats _getStatsInternal(String volume)
        {
            if (volume == null) throw new NullArgumentException("volume");

            VolumeQosStats stats = _volumeOps.get(volume);
            if (stats == null)
            {
                throw new IllegalArgumentException("No such volume: " + volume);
            }

            return stats;
        }
        
        private void _onOperationExecuted(Operation operation)
        {
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
        
        private void _onVolumeAdded(Volume volume)
        {
            String name = volume.getName();
            _volumeOps.put(name, new VolumeQosStats(volume));
        }
        
        private void _onVolumeModified(BeforeAfter<Volume> volume)
        {
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
        
        private void _onVolumeStart(Object volumeName)
        {
            VolumeQosStats stats = _volumeOps.get(volumeName);
            if (stats != null)
            {
                stats.performance.startNow();
            }
        }
        
        private void _onVolumeStatted(Volume volume)
        {
            String name = volume.getName();
            VolumeQosStats stats = _volumeOps.get(name);
            if (stats == null)
            {
                stats = new VolumeQosStats(VolumeQosSettings.fromVolume(volume));
            }
            else
            {
                stats = new VolumeQosStats(VolumeQosSettings.fromVolume(volume),
                                           stats.performance);
            }
            _volumeOps.put(name, stats);
        }
        
        private void _onVolumeStop(Object volumeName)
        {
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
    
    @Override
    public Context newContext(WorkloadEventListener listener)
    {
        if (listener == null) throw new NullArgumentException("listener");
        
        Context retval = new Context(listener);
        
        return retval;
    }
}
