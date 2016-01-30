/**
 *
 */
package com.formationds.om.repository.influxdb;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.stream.Collectors;

import org.influxdb.dto.Serie;

import com.formationds.client.v08.model.Volume;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.query.QueryCriteria;

/**
 * Implementation of the InfluxMetricRepoistory that uses a series per volume
 * instead of a single series that represents volume metadata as columns.
 *
 * @author dave
 */
public class InfluxMetricSeriesPerVolumeRepo extends InfluxMetricRepository {

    private static final String TAG_SEP = "&";
    private static final String DOMAIN_TOKEN = "domain=";
    private static final String VOLID_TOKEN = "&volid=";
    private static final String VOLNAME_TOKEN = "&volname=";
    private static final String METRICS_TOKEN = "&metrics";

    /**
     * domain.${volumeDomain}_volname.${volumeName}_volid.${volumeId}_metrics
     */
    public static final String SERIES_NAME_FMT = "domain=%s&volname=%s&volid=%d&metrics";

    public static final String SERIES_QUERY_FMT                   = "domain=%s&volname=%s&volid=%d&metrics";
    public static final String SERIES_QUERY_BY_VOLNAME_DOMAIN_FMT = "/domain=%s&volname=%s&volid=.*&metrics/";
    public static final String SERIES_QUERY_BY_VOLNAME_FMT        = "/domain=.*&volname=%s&volid=.*&metrics/";
    public static final String SERIES_QUERY_BY_VOLID_FMT          = "/domain=.*&volname=.*&volid=%d&metrics/";
    public static final String SERIES_QUERY_ALL_FOR_VOLDOMAIN_FMT = "/domain=%s&volname=.*&volid=.*&metrics/";
    public static final String SERIES_QUERY_ALL_VOL_FMT           = "/domain=.*&volname=.*&volid=.*&metrics/";

    // TODO: tenant id would be useful to have here too!  It is not in IVolumeDatapoint
    // or VolumeDatapoint so either need to add it there or look it up from cache...

    static final String querySeriesByVolumeName(String domain, String volumeName) {
        return String.format( SERIES_QUERY_BY_VOLNAME_DOMAIN_FMT, domain, volumeName );
    }

    static final String querySeriesByVolumeDomain(String domain) {
        return String.format( SERIES_QUERY_ALL_FOR_VOLDOMAIN_FMT, domain );
    }

    static final String querySeriesByVolumeName(String volumeName) {
        return String.format( SERIES_QUERY_BY_VOLNAME_FMT, volumeName );
    }

    static final String querySeriesByVolumeId(Long volumeId) {
        return String.format( SERIES_QUERY_BY_VOLID_FMT, volumeId );
    }

    static final String querySeriesAllVolumes() {
        return SERIES_QUERY_ALL_VOL_FMT;
    }

    public static final InfluxDatabase VOLUME_METRIC_DB =
            new InfluxDatabase.Builder( "om-volume-metricdb" )
            .addShardSpace( "default", "30d", "1d", "/.*/", 1, 1 )
            .build();

    /**
     * Get the metric names included in the volume metric series
     *
     * This only includes the timestamp column and metric names.
     * Volume metadata information is encoded in the series name.
     *
     * @return the list of metric names in the order they are stored.
     */
    private static List<String> getMetricNames() {
        List<String> metricNames = Arrays.stream( Metrics.values() )
                .map( Enum::name )
                .collect( Collectors.toList() );

        List<String> volMetricNames = new ArrayList<>();

        // time column necessary for writes where we want to force a timestamp
        // based on what we receive from the server side.  For queries
        // it is automatically returned and does not need to be included.
        volMetricNames.add( InfluxRepository.TIMESTAMP_COLUMN_NAME );

        // now add all of the metric names
        volMetricNames.addAll( metricNames );

        return volMetricNames;
    }

    /**
     * @param url
     * @param adminUser
     * @param adminCredentials
     */
    public InfluxMetricSeriesPerVolumeRepo( String url, String adminUser, char[] adminCredentials ) {
        super( url, adminUser, adminCredentials, VOLUME_METRIC_DB, getMetricNames(), Collections.emptyList() );
    }



    /* (non-Javadoc)
     * @see com.formationds.om.repository.influxdb.InfluxMetricRepository#getContextIdColumnName()
     */
    @Override
    public Optional<String> getContextIdColumnName() {
        return Optional.empty();
    }

    /* (non-Javadoc)
     * @see com.formationds.om.repository.influxdb.InfluxMetricRepository#getSeriesName(com.formationds.om.repository.influxdb.VolumeInfo)
     */
    @Override
    protected String getSeriesName( VolumeInfo v ) {
        return String.format( SERIES_NAME_FMT,
                              v.getDomainName() != null ? v.getDomainName() : "",
                              v.getVolumeName() != null ? v.getVolumeName() : "",
                              v.getVolumeId() );
    }

    /**
     * If the volume context list is empty (we'll also handle null), then
     * return the series filter for all volumes.
     *
     * if we have a single volume, use the volume information and return
     * as specific volume series name.
     *
     * If there are more than one volumes, we are going to select all
     * and post-process to filter the results.  The only other option I see
     * given the influxdb query language is to execute multiple queries
     * and I can't make that decision in this method.
     *
     * @param volumeContext
     *
     * @return the series filter to use
     */
    private String getSeriesQueryFilterForVolumeContext(List<Volume> volumeContext) {
        String seriesFilter;
        if ( volumeContext == null || volumeContext.isEmpty() ) {
            seriesFilter = querySeriesAllVolumes();
        } else if ( volumeContext.size() == 1 ) {
            Volume v = volumeContext.get( 0 );
            seriesFilter = querySeriesByVolumeId( v.getId() );
        } else {
            seriesFilter = querySeriesAllVolumes();
        }
        return seriesFilter;
    }

    /**
     * Override base implementation to apply volume filtering at the series name level
     * instead of as columns.
     *
     * @param queryCriteria
     *
     * @see com.formationds.om.repository.influxdb.InfluxRepository#formulateQueryString(com.formationds.om.repository.query.QueryCriteria, java.lang.String)
     */
    @Override
    protected String formulateQueryString( QueryCriteria queryCriteria ) {
        StringBuilder sb = new StringBuilder();

        // * if no columns, otherwise comma-separated list of column names
        String projection = queryCriteria.getColumnString();

        List<Volume> volumeContexts = queryCriteria.getContexts();
        sb.append( SELECT )
          .append( projection )
          .append( FROM )
          .append( getSeriesQueryFilterForVolumeContext( volumeContexts ) );

        if ( queryCriteria.getRange() != null ) {
            sb.append( WHERE );
        }

        // do time range
        if ( queryCriteria.getRange() != null ) {

            // TODO: InfluxDB queries might be slightly more efficient if we
            // use the built-in "now()" function with offsets instead of
            // absolute times.  It is definitely easier to understand
            // 'time > now() - 1d' at a glance
            // than 'time > 1450161444s and time < 1452753444s'
            DateRange range = queryCriteria.getRange();
            if ( range.getStart() == null )
                throw new IllegalArgumentException( "DateRange must have a start time" );

            if ( range.getEnd() != null )
                sb.append( "( " );

            String influxUnit = toInfluxUnits( range.getUnit() );
            sb.append( getTimestampColumnName() ).append( " > " )
              .append( range.getStart() )
              .append( influxUnit );

            if ( range.getEnd() != null ) {
                sb.append( AND )
                  .append( getTimestampColumnName() )
                  .append( " < " )
                  .append( range.getEnd() ).append( influxUnit )
                  .append( " )" );
            }
        }

        // NOTE: InfluxDB doesn't have a clean mechanism to support queryCriteria.getFirstPoint()
        // which was intended for paging data.  It supports TOP/BOTTON and FIRST/LAST, but both
        // operate only on a single column.
        if ( queryCriteria.getFirstPoint() != null && queryCriteria.getFirstPoint() > 0 ) {
            logger.debug("InfluxDB does not support a mechanism to handle paging via FirstPoint/Points");
        }

        // add a query limit on the points returned
        if ( queryCriteria.getPoints() != null && queryCriteria.getPoints() > 0 ) {
            sb.append( " limit " ).append( queryCriteria.getPoints() );
        }

        return sb.toString();
    }

    /**
     * Extract Volume metadata from the series name.
     *
     * @param s the series
     * @return VolumeInfo containing the domain name, volume name and volume id
     */
    // "domain=%s&volname=%s&volid=%d&metrics"
    private VolumeInfo extractVolumeMetadata(Serie s) {

        // Note: since format was changed to be key=value[&key2=value2[&...]]
        // we could change this to use split( '&' ) and then support additional "tags"
        // in the series name
        String sname = s.getName();

        int d = sname.indexOf( DOMAIN_TOKEN );
        int vn = sname.indexOf( VOLNAME_TOKEN );
        int vi = sname.indexOf( VOLID_TOKEN );

        int t = sname.indexOf( METRICS_TOKEN );

        //domain.blah_volname.test-vol1_volid.1;
        //0      7
        String domain = sname.substring( d + DOMAIN_TOKEN.length(), vn );
        String volName = sname.substring( vn + VOLNAME_TOKEN.length(), vi );
        String volIdStr = sname.substring( vi + VOLID_TOKEN.length(), t);

        return new VolumeInfo(domain, volName, Long.valueOf( volIdStr ) );
    }


    /**
     *
     * @param ts
     * @param volumeId
     * @param voldps
     * @return
     */
    protected Object[] convertPointsToSeriesRow( Long ts, VolumeInfo volumeInfo, List<IVolumeDatapoint> voldps ) {
        Object[] metricValues = new Object[getVolumeMetricColumnNames().size()];

        metricValues[0] = ts;

        for ( IVolumeDatapoint vdp : voldps ) {

            // TODO: assert that volume name in datapoint matches the volume info volume name.

            // find the metric position
            int midx = indexOf( vdp.getKey() );
            if (midx == -1) {
                // NOTE: We currently only populate InfluxDB with metrics explicitly defined in the
                // Metrics enum.  Additional metrics were recently added to the stat stream that we
                // are NOT writing to Influx at this time.
                logger.trace( "Metric {} not found in Volume Metrics list.  Skipping.", vdp.getKey() );
                continue;
            }
            metricValues[midx] = vdp.getValue();
        }

        return metricValues;
    }


    /**
     * Overrides the base implementation to account for volume metadata being in series
     * name instead of as columns in the metric data.
     *
     * @see com.formationds.om.repository.influxdb.InfluxMetricRepository#convertSeriesToPoints(com.formationds.om.repository.query.QueryCriteria, org.influxdb.dto.Serie)
     */
    @Override
    protected List<IVolumeDatapoint> convertSeriesToPoints( QueryCriteria criteria, Serie series ) {

        final List<IVolumeDatapoint> datapoints = new ArrayList<>();

        // we expect rows from one and only one series.  If there are more, we'll only use
        // the first one.  Note that this may change if we query using joins over multiple series
        // (we currently only have volume_metrics).
        if ( series == null || series.getRows().size() == 0 ) {
            return datapoints;
        }

        VolumeInfo vid = extractVolumeMetadata( series );
        List<Map<String, Object>> rowList = series.getRows();

        for ( Map<String, Object> row : rowList ) {

            // get the timestamp
            Object timestampO;

            try {
                timestampO = row.get( getTimestampColumnName() );
            } catch ( NoSuchElementException nsee ) {
                continue;
            }

            // we expect a value for all of these fields.  If not, bail
            if ( timestampO == null ) {
                continue;
            }

            Long timestamp = ((Number) timestampO).longValue();
            String volumeName = vid.getVolumeName();
            String volumeId = String.valueOf( vid.getVolumeId() );

            List<Volume> volumeContexts = criteria.getContexts();
            boolean postFilterVolumes = volumeContexts.size() > super.getVolumeContextFilterThreshold();
            if ( postFilterVolumes &&
                    volumeContexts.stream().anyMatch( (v) -> v.getId().equals( vid.getVolumeId() ) ) ) {
                continue;
            }

            row.forEach( ( key, value ) -> {

                // If we run across a column for metadata we just skip it.
                // we're only interested in the stats columns at this point
                try {
                    if ( super.isTimestampColumn( key ) ||
                            super.isSequenceColumn( key ) ||
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

                VolumeDatapoint point = new VolumeDatapoint( timestamp, volumeId, volumeName, key, numberValue );
                datapoints.add( point );
            } );
        } // for each row

        return datapoints;
    }
}
