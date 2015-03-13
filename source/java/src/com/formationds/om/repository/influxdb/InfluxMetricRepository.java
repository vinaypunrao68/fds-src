/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.entity.builder.VolumeDatapointBuilder;
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
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 *
 */
public class InfluxMetricRepository extends InfluxRepository<IVolumeDatapoint, Long> implements MetricRepository {

    public static final String VOL_SERIES_NAME = "volume_metrics";
    public static final String VOL_ID_COLUMN_NAME = "volume_id";
    public static final String TIMESTAMP_COLUMN_NAME = "time";

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
        volMetricNames.add( VOL_ID_COLUMN_NAME );
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

    @Override
    public String getEntityName() {
        return VOL_SERIES_NAME;
    }

    @Override
    public String getTimestampColumnName() {
        return TIMESTAMP_COLUMN_NAME;
    }

    @Override
    public Optional<String> getVolumeNameColumnName() {
        return Optional.empty();
    }

    @Override
    public Optional<String> getVolumeIdColumnName() {
        return Optional.of( VOL_ID_COLUMN_NAME );
    }

    /**
     * @throws UnsupportedOperationException persisting individual metrics is not supported for the
     * Influx Metric Repository
     */
    @Override
    protected VolumeDatapoint doPersist( IVolumeDatapoint entity ) {
        throw new UnsupportedOperationException( "Persisting individual metrics is not supported for the Influx Metric repository" );
    }

    @Override
    protected List<IVolumeDatapoint> doPersist( Collection<IVolumeDatapoint> entities ) {
        Object[] metricValues = new Object[ VOL_METRIC_NAMES.size() ];

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<IVolumeDatapoint> vdps = (entities instanceof List ? (List)entities : new ArrayList<>( entities ));

        // timestamp, map<volname, List<vdp>>>
        Map<Long, Map<String, List<IVolumeDatapoint>>> orderedVDPs;
        orderedVDPs = vdps.stream()
                           .collect( Collectors.groupingBy( IVolumeDatapoint::getTimestamp,
                                                            Collectors.groupingBy( IVolumeDatapoint::getVolumeName ) ) );

        for (Map.Entry<Long,Map<String,List<IVolumeDatapoint>>> e : orderedVDPs.entrySet())
        {
            Long ts = e.getKey();
            Map<String, List<IVolumeDatapoint>> volumeDatapoints = e.getValue();

            for (Map.Entry<String, List<IVolumeDatapoint>> e2 : volumeDatapoints.entrySet()) {
                String volid = e2.getKey();

                metricValues[0] = volid;

                for (IVolumeDatapoint vdp : e2.getValue())
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
    protected void doDelete( IVolumeDatapoint entity ) {
        // TODO: not supported yet
        throw new UnsupportedOperationException( "Delete not yet supported" );
    }

    @Override
    public VolumeDatapoint findById( Long aLong ) {
        // TODO: not sure how meaningful this is in the context of influxdb?
        // there is a generated sequence and a timestamp that effectively indicate the id, but
        // it seems pretty useless in this context.
        return null;
    }

    @Override
    public List<IVolumeDatapoint> findAll() {
        // pretty useless (and memory intensive)
        List<Serie> series = getConnection().getDBReader().query( "select * from " + VOL_SERIES_NAME, TimeUnit.MINUTES );

        // TODO: incomplete
        List<IVolumeDatapoint> results = new ArrayList<>( );
//        for (Serie s : series)
//        {
//            for (Map<String,Object> row : s.getRows()) {
//                VolumeDatapoint vdp = new VolumeDatapoint(  );
//                results.add( vdp );
//            }
//        }
        return results;
    }

    @Override
    public long countAll() {
        return 0;
    }

    @Override
    public long countAllBy( IVolumeDatapoint entity ) {
        return 0;
    }

    @Override
    public List<IVolumeDatapoint> query( QueryCriteria queryCriteria ) {
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
    public <VDP extends IVolumeDatapoint> VDP  mostRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP  mostRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP  leastRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP  leastRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics metric ) {
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
