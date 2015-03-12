/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.crud.AbstractRepository;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.query.QueryCriteria;
import org.influxdb.dto.Serie;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Properties;
import java.util.stream.Collectors;

/**
 *
 */
public class InfluxMetricRepository extends InfluxRepository<VolumeDatapoint, Long> implements MetricRepository {

    public static final String VOL_SERIES_NAME = "volume_metrics";

    /**
     * the static list of metric names store in the influxdb database.
     */
    public static final List<String> VOL_METRIC_NAMES = Collections.unmodifiableList( getMetricNames() );

    public static final InfluxDatabase DEFAULT_METRIC_DB =
        new InfluxDatabase.Builder( "om-metricdb" )
                          .addShardSpace( "default", "30d", "1d", "/.*/", 1, 1 )
                          .build();

    /**
     * @return the list of metric names in the order they are stored.
     */
    public static List<String> getMetricNames() {
        List<String> metricNames = Arrays.stream( Metrics.values() )
                                         .map( Enum::name )
                                         .collect( Collectors.toList() );

        List<String> volMetricNames = new ArrayList<>();
        volMetricNames.add( "volume_id" );
        volMetricNames.addAll( metricNames );

        return volMetricNames;
    }

    /**
     *
     * @param url
     * @param adminUser
     * @param adminCredentials
     */
    public InfluxMetricRepository( String url, String adminUser, char[] adminCredentials ) {
        super( url, adminUser, adminCredentials );
    }

    /**
     * @param properties the connection properties
     */
    @Override
    synchronized protected void open( Properties properties ) {
        // command is silently ignored if the database already exists.
        if ( !super.createDatabase( DEFAULT_METRIC_DB ) ) {
            logger.debug( "Database " + DEFAULT_METRIC_DB.getName() + " already exists." );
        }
        super.open( properties );
    }

    /**
     * @throws UnsupportedOperationException persisting individual metrics is not supported for the
     * Influx Metric Repository
     */
    @Override
    protected VolumeDatapoint doPersist( VolumeDatapoint entity ) {
        throw new UnsupportedOperationException( "Persisting individual metrics is not supported for the Influx Metric repository" );
    }

    @Override
    protected List<VolumeDatapoint> doPersist( Collection<VolumeDatapoint> entities ) {
        Object[] metricValues = new Object[ VOL_METRIC_NAMES.size() ];

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<VolumeDatapoint> vdps = (entities instanceof List ? (List)entities : new ArrayList<>( entities ));
        // timestamp, map<volname, List<vdp>>>
        Map<Long, Map<String, List<VolumeDatapoint>>> orderedVDPs;
        orderedVDPs = vdps.stream()
                           .collect( Collectors.groupingBy( VolumeDatapoint::getTimestamp,
                                                            Collectors.groupingBy( VolumeDatapoint::getVolumeName ) ) );

        for (Map.Entry<Long,Map<String,List<VolumeDatapoint>>> e : orderedVDPs.entrySet())
        {
            Long ts = e.getKey();
            Map<String, List<VolumeDatapoint>> volumeDatapoints = e.getValue();

            for (Map.Entry<String, List<VolumeDatapoint>> e2 : volumeDatapoints.entrySet()) {
                String volid = e2.getKey();

                metricValues[0] = volid;

                for (VolumeDatapoint vdp : e2.getValue())
                {
                    // TODO: figure out what metric it maps to, then figure out its position in
                    // the values array.
                }

                Serie serie = new Serie.Builder( VOL_SERIES_NAME )
                                  .columns( VOL_METRIC_NAMES.toArray( new String[VOL_METRIC_NAMES.size()] ) )
                                  .values()
                                  .build();
            }
        }
        return vdps;
    }

    @Override
    protected void doDelete( VolumeDatapoint entity ) {
        // TODO: not supported yet
        throw new UnsupportedOperationException( "Delete not yet supported" );
    }

    @Override
    public VolumeDatapoint findById( Long aLong ) {
        // TODO: not sure how meaningful this is in the context of influxdb?
        return null;
    }

    @Override
    public List<VolumeDatapoint> findAll() {
        return null;
    }

    @Override
    public long countAll() {
        return 0;
    }

    @Override
    public long countAllBy( VolumeDatapoint entity ) {
        return 0;
    }

    @Override
    public List<? extends VolumeDatapoint> query( QueryCriteria queryCriteria ) {
        return null;
    }

    @Override
    public Double sumLogicalBytes() {
        return null;
    }

    @Override
    public Double sumPhysicalBytes() {
        return null;
    }

    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
        return null;
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( Long volumeId ) {
        return null;
    }

    @Override
    public Optional<VolumeStatus> getLatestVolumeStatus( String volumeName ) {
        return null;
    }
}
