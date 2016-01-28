/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.om.repository.influxdb;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.EndUserMessages;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.helper.VolumeDatapointHelper;
import com.formationds.om.repository.query.OrderBy;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.QueryCriteria.QueryType;

import org.apache.thrift.TException;
import org.influxdb.dto.ChunkedResponse;
import org.influxdb.dto.Serie;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.CompletableFuture;
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

    public static final Logger logger = LoggerFactory.getLogger( InfluxMetricRepository.class );

    public static final String VOL_SERIES_NAME    = "volume_metrics";
    public static final String VOL_ID_COLUMN_NAME = "volume_id";
    public static final String VOL_DOMAIN_COLUMN_NAME = "volume_domain";
    public static final String VOL_NAME_COLUMN_NAME = "volume_name";

    public static final int DEFAULT_BATCH_SIZE = 1024;

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

        // time column necessary for writes where we want to force a timestamp
        // based on what we receive from the server side.  For queries it is automatically returned
        // and does not need to be included.
        volMetricNames.add( InfluxRepository.TIMESTAMP_COLUMN_NAME );

        // add the volume metadata columns
        addVolumeMetadataColumns( volMetricNames );

        // now add all of the metric names
        volMetricNames.addAll( metricNames );

        return volMetricNames;
    }

    /**
     * Add the volume metadata columns to the base list of columns.
     *
     * @param base the list to add the volume metadata columns to.  They are added to the end of the list.
     */
    protected static void addVolumeMetadataColumns( List<String> base ) {
        base.add( VOL_ID_COLUMN_NAME );
        base.add( VOL_DOMAIN_COLUMN_NAME );
        base.add( VOL_NAME_COLUMN_NAME );
    }

    private final VolumeMetricCache metricCache;

    private boolean batchedWritesEnabled;
    private int batchSize = DEFAULT_BATCH_SIZE;

    /**
     * @param url the url
     * @param adminUser the admin user
     * @param adminCredentials the credentials
     */
    public InfluxMetricRepository( String url, String adminUser, char[] adminCredentials ) {
        super( url, adminUser, adminCredentials );
        metricCache = new VolumeMetricCache( this );
        batchedWritesEnabled = FdsFeatureToggles.INFLUX_WRITE_BATCHING.isActive();
        batchSize = (batchedWritesEnabled ?
                               SingletonConfiguration.getIntValue( "fds.om.influxdb.batch_write_size",
                                                                   DEFAULT_BATCH_SIZE ) :
                               1 );
    }

    public void initializeCache() {
        super.whenConnected( () -> {
            List<Long> vids = getVolumeIds();
            logger.info("Initializing volume metric cache for {} volumes.", vids.size());
            metricCache.loadCache( vids );
            logger.info("Completed initializing volume metric cache. Size={}", metricCache.size());
        } );
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
    protected <R extends IVolumeDatapoint> R doPersist( R entity ) {
        throw new UnsupportedOperationException( "Persisting individual metrics is not " +
                "supported for the Influx Metric repository" );
    }

    /**
     * @param metricKey the metric key or enum name
     * @return the index of the specified metric key in the Volume metrics array.  -1 if not found
     */
    protected int indexOf( String metricKey ) {
        try {
            return VOL_METRIC_NAMES.indexOf( Metrics.lookup( metricKey ).name() );
        } catch (UnsupportedMetricException ume) {
            return -1;
        }
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    @Override
    protected <R extends IVolumeDatapoint> List<R> doPersist( Collection<R> entities ) {

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<IVolumeDatapoint> vdps = (entities instanceof List ? (List) entities : new ArrayList<>( entities ));

        // timestamp, map<volname, List<vdp>>>
        // TODO: why do we care if they are ordered?  Influx should handle that
        Map<Long, Map<String, List<IVolumeDatapoint>>> orderedVDPs =
                VolumeDatapointHelper.sortByTimestampAndVolumeId( vdps, true );

        SerieBuilder builder = newSerieBuilder();

        for ( Map.Entry<Long, Map<String, List<IVolumeDatapoint>>> e : orderedVDPs.entrySet() ) {

            Long ts = e.getKey();
            Map<String, List<IVolumeDatapoint>> volumeDatapoints = e.getValue();

            for ( Map.Entry<String, List<IVolumeDatapoint>> e2 : volumeDatapoints.entrySet() ) {

                Object[] metricValues = new Object[VOL_METRIC_NAMES.size()];

                // TODO: need volume ids as long everywhere
                Long volid = Long.valueOf( e2.getKey() );
                String volDomain = "";

                metricValues[0] = ts;
                metricValues[1] = volid;
                metricValues[2] = volDomain;

                List<IVolumeDatapoint> voldps = e2.getValue();
                for ( IVolumeDatapoint vdp : voldps ) {
                    metricValues[3] = vdp.getVolumeName();

                    // find the metric position
                    int midx = indexOf( vdp.getKey() );
                    if (midx == -1) {
                        logger.debug( "Metric {} not found in Volume Metrics list.  Skipping.", vdp.getKey() );
                        continue;
                    }
                    metricValues[midx] = vdp.getValue();
                }

                // update the metric cache
                metricCache.updateLatestVolumeStats( Long.valueOf( volid ),  voldps );

                int batchCount = builder.addRow(metricValues);

                if ( batchedWritesEnabled ) {

                    if ( batchCount >= batchSize ) {

                        // build (and reset) the batch
                        Serie writeBatch = builder.build();

                        // We are now explicitly specifying the timestamp and since VolumeDatapoint timestamps are
                        // in seconds since the epoch, specify a SECONDS precision on the series
                        getConnection().getAsyncDBWriter().write( TimeUnit.SECONDS, writeBatch );
                    }
                } else {

                    Serie writeSingle = builder.build();
                    getConnection().getAsyncDBWriter().write( TimeUnit.SECONDS, writeSingle );
                }
            }
        }

        // if there are any remaining entries in the batch, make sure they are written
        if ( batchedWritesEnabled && builder.getRowCount() > 0 ) {

            Serie writeBatch = builder.build();

            // We are now explicitly specifying the timestamp and since VolumeDatapoint timestamps are
            // in seconds since the epoch, specify a SECONDS precision on the series
            getConnection().getAsyncDBWriter().write( TimeUnit.SECONDS, writeBatch );
        }

        return (List<R>)vdps;
    }

    /**
     * Wrap a influxdb-java Serie.Builder to track the row count and reset the builder
     *
     * This class is not thread safe.
     */
    public static class SerieBuilder {

        private final String seriesName;
        private final List<String> columns;
        private Serie.Builder serieBuilder;
        private int rowCount = 0;

        public SerieBuilder( String seriesName, List<String> columns ) {
            super();
            this.seriesName = seriesName;
            this.columns = columns;
        }

        public int getRowCount() {
            return rowCount;
        }

        private Serie.Builder getSerieBuilder() {
            if (serieBuilder == null) {
                serieBuilder = new Serie.Builder( seriesName )
                        .columns( columns.toArray( new String[columns.size()] ) );
                rowCount = 0;
            }
            return serieBuilder;
        }

        /**
         *
         * @param metricValues
         * @return the current number of rows after addition
         */
        public int addRow( Object[] metricValues ) {
            getSerieBuilder().values( metricValues );
            return ++rowCount;
        }

        /**
         * Build the series from the current set of rows and reset the builder.
         *
         * @return the series.
         */
        public Serie build() {
            Serie s = getSerieBuilder().build();
            serieBuilder = null;
            return s;
        }
    }

    private SerieBuilder newSerieBuilder() {
        return new SerieBuilder( VOL_SERIES_NAME, VOL_METRIC_NAMES );
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

        if (series == null || series.isEmpty())
            return 0;

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
     * Convert an influxDB return type into VolumeDatapoints that we can use
     *
     * @param criteria the criteria that was used to query the database.
     * @param series the series to convert.  possibly null
     *
     * @return the list of volume data points from the series.  empty list if the series is null
     */
    protected List<IVolumeDatapoint> convertSeriesToPoints( QueryCriteria criteria, List<Serie> series ) {

        final List<IVolumeDatapoint> datapoints = new ArrayList<>();
        if (series == null) {
            return datapoints;
        }

        for (Serie s : series) {
            datapoints.addAll( convertSeriesToPoints( criteria, s ) );
        }
        return datapoints;
    }

    /**
     *
     * @param criteria the query criteria
     * @param chunkedResponse the chunked query response.  possibly null.
     *
     * @return the list of datapoints. empty list if the chunked response is null
     */
    protected List<IVolumeDatapoint> convertSeriesToPoints( QueryCriteria criteria, ChunkedResponse chunkedResponse ) {

        final List<IVolumeDatapoint> datapoints = new ArrayList<>();

        if (chunkedResponse == null)
            return datapoints;

        Serie series = null;
        while ((series = chunkedResponse.nextChunk()) != null) {
            datapoints.addAll( convertSeriesToPoints( criteria, series ) );
        }
        return datapoints;
    }

    /**
     *
     * @param criteria the query criteria
     * @param series the individual series.  possibly null
     *
     * @return the list of datapoints. empty list if the chunked response is null.
     */
    protected List<IVolumeDatapoint> convertSeriesToPoints( QueryCriteria criteria, Serie series ) {

        final List<IVolumeDatapoint> datapoints = new ArrayList<>();

        // we expect rows from one and only one series.  If there are more, we'll only use
        // the first one.  Note that this may change if we query using joins over multiple series
        // (we currently only have volume_metrics).
        if ( series == null || series.getRows().size() == 0 ) {
            logger.trace("Series results from {} query was null or empty", criteria.getQueryType());
            return datapoints;
        }

        List<Map<String, Object>> rowList = series.getRows();

        logger.trace("Processing series '{}' with columns={} and {} rows",
                     series.getName(), Arrays.toString( series.getColumns() ),
                     rowList.size() );


        for ( Map<String, Object> row : rowList ) {

            // get the timestamp
            Object timestampO;
            Object volumeIdO;
            Object volumeNameO;

            try {
                timestampO = row.get( getTimestampColumnName() );
                volumeIdO = row.get( getVolumeIdColumnName().get() );
                volumeNameO = row.get( getVolumeNameColumnName().get() );
            } catch ( NoSuchElementException nsee ) {
                logger.warn( "Failed to locate expected metadata row: " + nsee.getMessage() );
                continue;
            }

            // we expect a value for all of these fields.  If not, bail
            if ( timestampO == null || volumeIdO == null || volumeNameO == null ) {
                logger.warn( "Failed to locate expected metadata row: ts={}; voldId={}; volName={}",
                             timestampO, volumeIdO, volumeNameO );
                continue;
            }

            Long timestamp = ((Number) timestampO).longValue();
            String volumeName = volumeNameO.toString();
            Long volumeId = ((Number)volumeIdO).longValue();
            String volumeIdStr = String.valueOf(volumeId);

            List<Volume> volumeContexts = criteria.getContexts();
            boolean postFilterVolumes = volumeContexts.size() > super.getVolumeContextFilterThreshold();
            if ( postFilterVolumes &&
                    ! volumeContexts.stream()
                                    .anyMatch( (v) -> v.getId().equals( volumeId ) ) ) {
                logger.trace( "Volume id={} not found in requested context list. Skipping",
                              volumeId );
                continue;
            }

            row.forEach( ( key, value ) -> {

                // If we run across a column for metadata we just skip it.
                // we're only interested in the stats columns at this point
                try {
                    if ( super.isTimestampColumn( key ) ||
                            super.isSequenceColumn( key ) ||
                            key.equals( getVolumeIdColumnName().get() ) ||
                            key.equals( getVolumeNameColumnName().get() ) ||
                            key.equals( getVolumeDomainColumnName().get() ) ||
                            value == null ) {
                        return;
                    }
                } catch ( NoSuchElementException nsee ) {
                    return;
                }

                // values may come back as either strings or numeric.
                // if they are numeric, it will likely be faster to cast
                // than to parse the string representation.  If they are
                // strings... well, we have an extra if check.
                Double numberValue = null;
                if (value instanceof Number) {
                    numberValue = ((Number)value).doubleValue();
                } else {
                    numberValue = Double.parseDouble( value.toString() );
                }

                VolumeDatapoint point = new VolumeDatapoint( timestamp, volumeIdStr, volumeName, key, numberValue );
                datapoints.add( point );
            } );
        } // for each row

        logger.trace( "Completed processing series '{}' aand {} rows.  Returning {} datapoints.",
                      series.getName(),
                      rowList.size(),
                      datapoints.size() );
        return datapoints;
    }

    @Override
    public List<IVolumeDatapoint> query( QueryCriteria queryCriteria ) {

        // get the query string
        String queryString = formulateQueryString( queryCriteria, getVolumeIdColumnName().get(), getVolumeNameColumnName().get() );

        List<IVolumeDatapoint> datapoints = null;

        // execute the query
        InfluxDBConnection conn = getConnection();
        InfluxDBReader reader = conn.getDBReader();
        if ( conn.isChunkedResponseEnabled() && reader.suportsChunkedResponseQuery() ) {
            try (ChunkedResponse response = reader.chunkedReponseQuery( queryString, TimeUnit.SECONDS ) ) {
                datapoints = convertSeriesToPoints( queryCriteria, response );
            }
        } else {
            List<Serie> series = reader.query( queryString, TimeUnit.SECONDS );

            // convert from influxdb format to FDS model format
            datapoints = convertSeriesToPoints( queryCriteria, series );
        }

        return datapoints;
    }

    /**
     *
     * @return the list of volume ids
     */
    protected List<Long> getVolumeIds() {
        try {
            // expect this is fully cached.
            List<VolumeDescriptor> volumeDescriptors = SingletonConfigAPI.instance().api().listVolumes( "" );

            return volumeDescriptors.stream().map( VolumeDescriptor::getVolId ).collect( Collectors.toList() );

        } catch (TException te ) {
            throw new IllegalStateException( EndUserMessages.CS_ACCESS_DENIED, te );
        }
    }

    @Override
    public Double sumLogicalBytes() {
        return sumMetric( Metrics.LBYTES );
    }

    @Override
    public Double sumPhysicalBytes() {
        return sumMetric( Metrics.UBYTES );
    }

    protected Double sumMetric( Metrics metrics ) {
        final List<IVolumeDatapoint> datapoints = new ArrayList<>();
        final List<Long> volumeIds = getVolumeIds();
        volumeIds.stream().forEach( ( vId ) -> {
            final List<IVolumeDatapoint> vdps = mostRecentOccurrenceBasedOnTimestamp( vId, metrics );
            if ( !vdps.isEmpty() ) {
                final IVolumeDatapoint vdp = vdps.get( 0 );
                if ( vdp != null ) {
                    datapoints.add( vdp );
                }
            }
        } );

        return datapoints.stream()
                .mapToDouble( IVolumeDatapoint::getValue )
                .summaryStatistics()
                .getSum();
    }

    /**
     *
     * @param volumeId the volume to query
     * @param metrics the list of metrics to query.  If null or empty, all metrics are returned.
     *
     * @return the list of datapoints matching the metrics for the specified volume.  If the metrics are null or empty
     *  all available metrics are returned.
     */
    @Override
    public List<IVolumeDatapoint> mostRecentOccurrenceBasedOnTimestamp( Long volumeId, EnumSet<Metrics> metrics) {

        // if the cache is empty, submit an async task that will pre-load it.
        // This volume's metrics will most likely be loaded by a separate query via
        // the cache's metric loader.
        if ( metricCache.isEmpty() ) {
            CompletableFuture.runAsync( () -> {
                try {
                    logger.trace( "Attempting to pre-load volume metric cache" );
                    List<Long> volids = getVolumeIds();

                    metricCache.loadCache( volids );
                } catch ( Exception te ) {
                    logger.trace( "Failed to pre-load volume metric cache", te );
                }
            }
                    );
        }

        EnumMap<Metrics, IVolumeDatapoint> m = metricCache.getLatestVolumeStats( volumeId, metrics );
        return VolumeMetricCache.toVolumeDatapoints( m );
    }

    /**
     * Load the most recent stats for the specified volume from the influx repository.  This does not access the
     * cache (but is used to initialize the cache)
     *
     * @param volumeId the volume id
     *
     * @return the most recent stats for the volume
     */
    List<IVolumeDatapoint> loadMostRecentVolumeStats( Long volumeId ) {

        QueryCriteria queryCriteria = new QueryCriteria( QueryType.VOLUME_LATEST_METRICS );

        // This could be used to select only volume metadata columns and specific metrics.  For now I think we
        // just want all of the latest....
        //        if ( metrics != null && metrics.length > 0) {
        //            List<String> metricNames = Arrays.stream( metrics )
        //                                             .map( Enum::name )
        //                                             .collect( Collectors.toList() );
        //            addVolumeMetadataColumns( metricNames );
        //            queryCriteria.addColumns(metricNames);
        //        }

        // this only works because we know that formulateQueryString uses the volume id in the query.
        queryCriteria.setContexts( Collections.singletonList( new Volume( volumeId, volumeId.toString() ) ) );
        queryCriteria.addOrderBy( new OrderBy(getTimestampColumnName(), false) );
        queryCriteria.setPoints(1);

        // get the query string
        String queryString = formulateQueryString( queryCriteria, getVolumeIdColumnName().get(), getVolumeNameColumnName().get() );

        // execute the query -  limiting it to the most recent row for the volume is now done in
        // formulateQueryString
        List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.SECONDS );

        if (series.isEmpty()) {
            return Collections.emptyList();
        }

        // convert from influxdb format to FDS model format
        List<IVolumeDatapoint> datapoints = convertSeriesToPoints( queryCriteria, series );

        return datapoints;

    }
}
