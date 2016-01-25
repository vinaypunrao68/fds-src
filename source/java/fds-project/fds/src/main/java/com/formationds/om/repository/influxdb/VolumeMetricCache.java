/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
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

/**
 * Caches the latest set of stats for each volume.
 */
public class VolumeMetricCache {

    public static final Logger logger = LoggerFactory.getLogger( VolumeMetricCache.class );

    /**
     * Helper to convert from the internal EnumMap representation to a list of VolumeDatapoints
     *
     * @param m an EnumMap representation of metrics
     *
     * @return a list of volume datapoints.  Order is not guaranteed, but is generally expected to match the
     * ordinal values of metrics (which is based on the order they are defined in the Metrics enum).
     */
    public static List<IVolumeDatapoint> toVolumeDatapoints( EnumMap<Metrics, IVolumeDatapoint> m ) {
        return new ArrayList<>( m.values() );
    }

    /**
     * Convert the list of Volume Datapoints to the internal representation of the metrics enum map
     * as stored in the cache.
     *
     * @param vdps the list of datapoints
     *
     * @return the enum map representation.
     */
    protected static EnumMap<Metrics, IVolumeDatapoint> toEnumMap(List<IVolumeDatapoint> vdps) {
        EnumMap<Metrics, IVolumeDatapoint> result = new EnumMap<>( Metrics.class );
        vdps.forEach( ( vdp ) -> {
            try {
                result.put( Metrics.lookup( vdp.getKey() ), vdp );
            } catch (UnsupportedMetricException e) {
                logger.debug( "Metric {} not found in Volume Metrics list.  Skipping.", vdp.getKey() );
            }
        } );
        return result;
    }

    /**
     * Mapping of volume id to the latest set of metrics for that volume
     */
    private final Map<Long, EnumMap<Metrics, IVolumeDatapoint>> mostRecentVolumeDatapoints = new ConcurrentHashMap<>();

    /**
     * The repository instance used by the #volumeMetricLoader function
     */
    private final InfluxMetricRepository                             metricRepository;

    /**
     * A function for loading the latest stats for a specific volume id from the repository
     */
    private final Function<Long, EnumMap<Metrics, IVolumeDatapoint>> volumeMetricLoader;

    /**
     *
     * @param repository the influx metric repository for loading stats as necessary.
     */
    public VolumeMetricCache( InfluxMetricRepository repository ) {
        metricRepository = repository;
        volumeMetricLoader = ( volumeId ) -> {
            List<IVolumeDatapoint> vdps = metricRepository.loadMostRecentVolumeStats( volumeId );
            return toEnumMap( vdps );
        };
    }

    /**
     * @return true if the cache is empty
     */
    public boolean isEmpty() { return mostRecentVolumeDatapoints.isEmpty(); }

    /**
     * @return the current number of volumes loaded in the cache.
     */
    public int size() { return mostRecentVolumeDatapoints.size(); }

    /**
     * Load the specified volume ids into the cache.
     *
     * @param volumeIds volume ids to load
     */
    public void loadCache( List<Long> volumeIds ) {
        volumeIds.stream().forEach( ( vid ) -> {
            mostRecentVolumeDatapoints.put( vid, volumeMetricLoader.apply( vid ) );
        } );
    }

    /**
     * Clear the cache
     */
    public void clear() {
        mostRecentVolumeDatapoints.clear();
    }

    /**
     * Clear the specified volume ids from the cache
     *
     * @param volumeIds
     */
    public void clear( List<Long> volumeIds ) {
        volumeIds.stream().forEach( mostRecentVolumeDatapoints::remove );
    }

    /**
     * Update the latest volume stats with the specified datapoints
     *
     * @param volumeId the volume to update
     * @param vdps the latest stats
     */
    void updateLatestVolumeStats( Long volumeId, List<IVolumeDatapoint> vdps ) {
        EnumMap<Metrics, IVolumeDatapoint> m = toEnumMap( vdps );
        updateLatestVolumeStats( volumeId, m );
    }

    /**
     * Update the latest volume stats with the specified datapoints
     *
     * @param volumeId the volume to update
     * @param vdps the latest stats
     */
    // TODO: validate that the timestamp of the stats is actually greater than the currently stored timestamp?
    void updateLatestVolumeStats( Long volumeId, EnumMap<Metrics, IVolumeDatapoint> vdps ) {
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
     * Get the latest stats for the specified volume, loading them if necessary through the volume metric loader
     *
     * @param volumeId the id of the volume to load
     * @param metrics any additional metrics
     *
     * @return the Map of metrics to volume datapoint values
     */
    public EnumMap<Metrics, IVolumeDatapoint> getLatestVolumeStats( Long volumeId, EnumSet<Metrics> metrics ) {

        EnumMap<Metrics,IVolumeDatapoint> m = getLatestVolumeStats( volumeId ).clone();
        m.entrySet().removeIf( (kv) ->  !metrics.contains( kv.getKey() ) );

        return m;
    }
}
