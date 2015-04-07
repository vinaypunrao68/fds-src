/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository.influxdb;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.query.QueryCriteria;
import org.influxdb.dto.Serie;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Properties;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * Represent the metric repository in InfluxDB.
 * <p/>
 * The metrics database is named "om-metricdb", and the volume metrics table (series) is named
 * volume+metrics.  Each row contains volume metadata, followed by the metrics sent from the
 * formation server as represented by the {@link Metrics} enumeration.
 * <p/>
 * The volume metadata included is
 * <ul>
 *     <li>volume_id</li>
 *     <li>volume_domain</li>
 *     <li>volume_name</li>
 * </ul>
 */
public class InfluxMetricRepository extends InfluxRepository<IVolumeDatapoint, Long> implements MetricRepository {

    public static final String VOL_SERIES_NAME    = "volume_metrics";
    public static final String VOL_ID_COLUMN_NAME = "volume_id";
    public static final String VOL_DOMAIN_COLUMN_NAME = "volume_domain";
    public static final String VOL_NAME_COLUMN_NAME = "volume_name";

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
        volMetricNames.add( VOL_DOMAIN_COLUMN_NAME );
        volMetricNames.add( VOL_NAME_COLUMN_NAME );
        volMetricNames.addAll( metricNames );

        return volMetricNames;
    }

    /**
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
    synchronized public void open( Properties properties ) {

        super.open( properties );

        // command is silently ignored if the database already exists.
        super.createDatabaseAsync( DEFAULT_METRIC_DB );
    }

    @Override
    public String getInfluxDatabaseName() { return DEFAULT_METRIC_DB.getName(); }

    @Override
    public String getEntityName() {
        return VOL_SERIES_NAME;
    }

    @Override
    public String getTimestampColumnName() {
        return super.getTimestampColumnName();
    }

    @Override
    public Optional<String> getVolumeNameColumnName() {
        return Optional.of( VOL_NAME_COLUMN_NAME );
    }

    @Override
    public Optional<String> getVolumeIdColumnName() {
        return Optional.of( VOL_ID_COLUMN_NAME );
    }
    
    @Override
    public Optional<String> getVolumeDomainColumnName() {
    	return Optional.of( VOL_DOMAIN_COLUMN_NAME );
    }

    /**
     * @throws UnsupportedOperationException persisting individual metrics is not supported for the
     *                                       Influx Metric Repository
     */
    @Override
    protected VolumeDatapoint doPersist( IVolumeDatapoint entity ) {
        throw new UnsupportedOperationException( "Persisting individual metrics is not supported for the Influx Metric repository" );
    }

    @Override
    protected <R extends IVolumeDatapoint> List<R> doPersist( Collection<R> entities ) {
        Object[] metricValues = new Object[VOL_METRIC_NAMES.size()];

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<R> vdps = (entities instanceof List ? (List) entities : new ArrayList<>( entities ));

        // timestamp, map<volname, List<vdp>>>
        Map<Long, Map<String, List<IVolumeDatapoint>>> orderedVDPs;
        orderedVDPs = vdps.stream()
                          .collect( Collectors.groupingBy( IVolumeDatapoint::getTimestamp,
                                                           Collectors.groupingBy( IVolumeDatapoint::getVolumeId ) ) );

        for ( Map.Entry<Long, Map<String, List<IVolumeDatapoint>>> e : orderedVDPs.entrySet() ) {
            Long ts = e.getKey();
            Map<String, List<IVolumeDatapoint>> volumeDatapoints = e.getValue();

            for ( Map.Entry<String, List<IVolumeDatapoint>> e2 : volumeDatapoints.entrySet() ) {
                String volid = e2.getKey();
                String volDomain = "";

                // volName is in the list of volume datapoints
                String volName = null;

                metricValues[0] = volid;
                metricValues[1] = volDomain;

                int v = 3;
                for ( IVolumeDatapoint vdp : e2.getValue() ) {
                    // TODO: figure out what metric it maps to, then figure out its position in
                    // the values array.
                    metricValues[2] = vdp.getVolumeName();
                    metricValues[v++] = vdp.getValue();
                }

                Serie serie = new Serie.Builder( VOL_SERIES_NAME )
                                  .columns( VOL_METRIC_NAMES.toArray( new String[VOL_METRIC_NAMES.size()] ) )
                                  .values(metricValues)
                                  .build();

                getConnection().getAsyncDBWriter().write( TimeUnit.MILLISECONDS, serie );
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
    public long countAll() {
        // execute the query
        List<Serie> series = getConnection().getDBReader()
                                            .query( "select count(PUTS) from " + getEntityName(),
                                                    TimeUnit.MILLISECONDS );

        Object o = series.get( 0 ).getRows().get(0).get( "count" ) ;
        if (o instanceof Number)
            return ((Number)o).longValue();
        else
            throw new IllegalStateException( "Invalid type returned from select count query" );
    }

    @Override
    public long countAllBy( IVolumeDatapoint entity ) {
        return 0;
    }

    /**
     * Method to create a string from the query object that matches influx format
     *
     * @param queryCriteria
     *
     * @return
     */
    protected String formulateQueryString( QueryCriteria queryCriteria ) {

        StringBuilder sb = new StringBuilder();

        String prefix = SELECT + " * " + FROM + " " + getEntityName();
        sb.append( prefix );

        if ( queryCriteria.getRange() != null ||
             queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {

            sb.append( " " + WHERE );
        }

        // do time range
        if ( queryCriteria.getRange() != null ) {

            String time = " ( " + getTimestampColumnName() + " > " + queryCriteria.getRange().getStart() + "s " + AND +
                          " " + getTimestampColumnName() + " < " + queryCriteria.getRange().getEnd() + "s ) ";

            sb.append( time );
        }

        if ( queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {

            sb.append( AND + " ( " );

            Iterator<Volume> contextIt = queryCriteria.getContexts().iterator();

            while ( contextIt.hasNext() ) {

                Volume volume = contextIt.next();

                sb.append( getVolumeIdColumnName().get() + " = '" + volume.getId() + "'" );

                if ( contextIt.hasNext() ) {
                    sb.append( " " + OR + " " );
                }
            }

            sb.append( " )" );
        }

        return sb.toString();
    }

    /**
     * Convert an influxDB return type into VolumeDatapoints that we can use
     *
     * @param series
     *
     * @return
     */
    protected List<IVolumeDatapoint> convertSeriesToPoints( List<Serie> series ) {

        final List<IVolumeDatapoint> datapoints = new ArrayList<IVolumeDatapoint>();

        // we expect rows from one and only one series.  If there are more, we'll only use
        // the first one
        if ( series == null || series.size() == 0 ) {
            return datapoints;
        }

        if (series.size() > 1) {
            logger.warn( "Expecting only one metric series.  Skipping " + (series.size() - 1) + " unexpected series." );
        }

        List<Map<String, Object>> rowList = series.get( 0 ).getRows();

        for ( Map<String, Object> row : rowList ) {

            // get the timestamp
            Object timestampO = null;
            Object volumeIdO = null;
            Object volumeNameO = null;

            try {
                timestampO = row.get( getTimestampColumnName() );
                volumeIdO = row.get( getVolumeIdColumnName().get() );
                volumeNameO = row.get( getVolumeNameColumnName().get() );
            } catch ( NoSuchElementException nsee ) {
                continue;
            }

            // we expect a value for all of these fields.  If not, bail
            if ( timestampO == null || volumeIdO == null || volumeNameO == null ) {
                continue;
            }

            Long timestamp = ((Double) timestampO).longValue();
            String volumeName = volumeIdO.toString();
            String volumeId = volumeIdO.toString();

            row.forEach( ( key, value ) -> {

                // If we run across a column for metadata we just skip it.
                // we're only interested in the stats columns at this point
                try {
                    if ( key.equals( getTimestampColumnName() ) ||
                         key.equals( getVolumeIdColumnName().get() ) ||
                         key.equals( getVolumeNameColumnName().get() ) ||
                         key.equals( getVolumeDomainColumnName().get() ) ||
                         value == null ) {
                        return;
                    }
                } catch ( NoSuchElementException nsee ) {
                    return;
                }

                Double numberValue = Double.parseDouble( value.toString() );

                VolumeDatapoint point = new VolumeDatapoint( timestamp, volumeId, volumeName, key, numberValue );
                datapoints.add( point );
            } );
        } // for each row

        return datapoints;
    }

    @Override
    public List<IVolumeDatapoint> query( QueryCriteria queryCriteria ) {

        // get the query string
        String queryString = formulateQueryString( queryCriteria );

        // execute the query
        List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.SECONDS );

        // convert from influxdb format to FDS model format
        List<IVolumeDatapoint> datapoints = convertSeriesToPoints( series );

        return datapoints;
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
    public <VDP extends IVolumeDatapoint> VDP mostRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                                                    Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP mostRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP leastRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics metric ) {
        return null;
    }

    @Override
    public <VDP extends IVolumeDatapoint> VDP leastRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                                                     Metrics metric ) {
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
