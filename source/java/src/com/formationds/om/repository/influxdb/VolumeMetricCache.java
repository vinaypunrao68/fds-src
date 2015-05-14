/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;

public class VolumeMetricCache {

    public static final Logger logger = LoggerFactory.getLogger( VolumeMetricCache.class );

    // TODO: using the post-persist listener may not be necessary because much of this work is already done
    // in the InfluxMetricRepository when persisting the data to begin with.  Breaking it apart and sorting/filtering
    // it again is extra work that is not necessary.  On the other hand, doing it in the IMR potentially means
    // more frequent updates to the cache only because of the way that it is processing.
//    class VolumeMetricCacheUpdater implements EntityPersistListener<IVolumeDatapoint> {
//
//        @Override
//        public <R extends IVolumeDatapoint> void postPersist( Collection<R> entities ) {
//            // may include datapoints from multiple volumes and different timestamps.  need to split
//            List<IVolumeDatapoint> datapoints = (entities instanceof List ?
//                                                    (List<IVolumeDatapoint>) entities :
//                                                    new ArrayList<>( entities ));
//
//            // reverse sort (desc) by timestamp so we can grab the most recent
//            SortedMap<Long, Map<String, List<IVolumeDatapoint>>> datapointsByVolumeIdAndTS =
//                VolumeDatapointHelper.sortByTimestampAndVolumeId( datapoints, false );
//
//            // TODO: this sorts then grabs the first (most recent) timestamp.  The only problem is that I don't know
//            // if we have any guarantees that we'll receive stats for all volumes at once.
//            Optional<Long> maxTimestamp = datapointsByVolumeIdAndTS.keySet()
//                                                              .stream()
//                                                              .findFirst();
//
//            if ( ! maxTimestamp.isPresent() ) {
//                // yikes! what happened?
//                logger.warn("No datapoints available to cache... This is not expected may indicate a logical failure " +
//                            "in the processing of incoming datapoints or a change in the way the datapoints are sent.");
//                return;
//            }
//
//            Map<String, List<IVolumeDatapoint>> mostRecentVolStats = datapointsByVolumeIdAndTS.get( maxTimestamp .get() );
//            mostRecentVolStats.entrySet().stream().forEach( (kv) -> {
//                Long volId = Long.valueOf( kv.getKey() );
//                VolumeMetricCache.this.updateLatestVolumeStats( volId, kv.getValue() );
//            });
//        }
//    }

    /**
     * Helper to convert from the internal EnumMap representation to a list of VolumeDatapoints
     *
     * @param m an EnumMap representation of metrics
     *
     * @return a list of volume datapoints.  Order is not guaranteed, but is generally expected to match the
     * ordinal values of metrics (which is based on the order they are defined in the Metrics enum).
     */
    public static List<IVolumeDatapoint> toVolumeDatapoints( EnumMap<Metrics, IVolumeDatapoint> m ) {
        List<IVolumeDatapoint> vdps = new ArrayList<>( m.values() );
        return vdps;
    }

    /**
     * Convert the list of Volume Datapoints to the internal representation of the metrics enum map
     * as stored in the cache.
     * @param vdps the list of datapoints
     * @return the enum map representation.
     */
    protected static EnumMap<Metrics, IVolumeDatapoint> toEnumMap(List<IVolumeDatapoint> vdps) {
        EnumMap<Metrics, IVolumeDatapoint> result = new EnumMap<>( Metrics.class );
        vdps.forEach( ( vdp ) -> result.put( Metrics.lookup( vdp.getKey() ), vdp ) );
        return result;
    }

    // TODO: maybe map to EnumMap<Metrics,IVolumeDatapoint> instead?  easier/more efficient lookup by specific metric
    // Downside is converting between that and the List expected by many existing apis
    private final Map<Long, EnumMap<Metrics, IVolumeDatapoint>> mostRecentVolumeDatapoints = new ConcurrentHashMap<>();

    private final InfluxMetricRepository metricRepository;
    private final Function<Long, EnumMap<Metrics, IVolumeDatapoint>> volumeMetricLoader;

    public VolumeMetricCache( InfluxMetricRepository repository ) {
        metricRepository = repository;
        volumeMetricLoader = (volumeId) -> {
            List<IVolumeDatapoint> vdps = metricRepository.loadMostRecentVolumeStats( volumeId );
            return toEnumMap( vdps );
        };
        // See note about VolumeMetricCacheUpdater above
        //    metricRepository.addEntityPersistListener( new VolumeMetricCacheUpdater() );
    }

    /**
     * Update the latest volume stats with the specified datapoints
     *
     * @param volumeId the volume to update
     * @param vdps the latest stats
     */
    void updateLatestVolumeStats( Long volumeId, List<IVolumeDatapoint> vdps ) {
        EnumMap<Metrics,IVolumeDatapoint> m = toEnumMap( vdps );
        updateLatestVolumeStats( volumeId, m );
    }

    /**
     * Update the latest volume stats with the specified datapoints
     *
     * @param volumeId the volume to update
     * @param vdps the latest stats
     */
    // TODO: validate that the timestamp of the stats is actually greater than the currently stored timestamp?
    void updateLatestVolumeStats( Long volumeId, EnumMap<Metrics,IVolumeDatapoint> vdps ) {
        mostRecentVolumeDatapoints.put( volumeId, vdps );
    }

    /**
     * Retrieve the latest volume stats for the specified volume.  If the stats for the volume have not yet
     * been loaded, this will automatically load them through the metric repository loader.
     *
     * @param volumeId the volume to load
     *
     * @return the latest stats for the specified volume.
     */
    public EnumMap<Metrics, IVolumeDatapoint> getLatestVolumeStats( Long volumeId ) {
        return mostRecentVolumeDatapoints.computeIfAbsent( volumeId, volumeMetricLoader );
    }

    /**
     *
     * @param volumeId
     * @param m1
     * @param metrics
     * @return
     */
    public EnumMap<Metrics, IVolumeDatapoint> getLatestVolumeStats( Long volumeId, Metrics m1, Metrics... metrics ) {
        EnumSet<Metrics> metricsEnumSet = (metrics != null ? EnumSet.of( m1, metrics ) : EnumSet.of( m1 ));
        return getLatestVolumeStats( volumeId, metricsEnumSet );
    }

    /**
     * Get the latest stats for the specified volume.
     *
     * @param volumeId
     * @param metrics
     *
     * @return
     */
    public EnumMap<Metrics, IVolumeDatapoint> getLatestVolumeStats( Long volumeId, EnumSet<Metrics> metrics ) {

        EnumMap<Metrics,IVolumeDatapoint> m = getLatestVolumeStats( volumeId ).clone();
        m.entrySet().removeIf( (kv) ->  !metrics.contains( kv.getKey() ) );

        return m;
    }
}
