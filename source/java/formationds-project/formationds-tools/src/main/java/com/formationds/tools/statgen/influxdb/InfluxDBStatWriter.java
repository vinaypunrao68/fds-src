/**
 *
 */
package com.formationds.tools.statgen.influxdb;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.influxdb.dto.Serie;

import com.formationds.tools.statgen.Metrics;
import com.formationds.tools.statgen.StatWriter;
import com.formationds.tools.statgen.VolumeDatapoint;
import com.formationds.tools.statgen.VolumeDatapointHelper;

/**
 * @author dave
 *
 */
public class InfluxDBStatWriter implements StatWriter {

    public static final String TIMESTAMP_COLUMN_NAME = "time";
    public static final String SEQUENCE_COLUMN_NAME = "sequence_number";
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
        volMetricNames.add( TIMESTAMP_COLUMN_NAME );

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

    InfluxDBConnection dbConnection;
    String user;
    String pwd;
    String db;

    /**
     *
     */
    public InfluxDBStatWriter(String url, String user, String pwd, String db) {
        this.user = user;
        this.pwd = pwd;
        this.db = db;
        dbConnection = new InfluxDBConnection( url, user, pwd.toCharArray(), db );
    }

    /* (non-Javadoc)
     * @see com.formationds.tools.statgen.StatWriter#sendStats(java.util.List)
     */
    @Override
    public void sendStats( List<VolumeDatapoint> vdps ) {
        doPersist( vdps );
    }

    /**
     * @return the dbConnection (null if not open)
     */
    public InfluxDBConnection getConnection() {
        return this.dbConnection;
    }


    /**
     * @param metricKey the metric key or enum name
     * @return the index of the specified metric key in the Volume metrics array.  -1 if not found
     */
    protected int indexOf( String metricKey ) {
        try {
            return VOL_METRIC_NAMES.indexOf( Metrics.lookup( metricKey ).name() );
        } catch (IllegalArgumentException ume) {
            return -1;
        }
    }

    protected List<VolumeDatapoint> doPersist( Collection<VolumeDatapoint> entities ) {
        Object[] metricValues = new Object[VOL_METRIC_NAMES.size()];

        // TODO: currently the collection of VolumeDatapoint objects is a list of individual data points
        // and may contain any number of volumes and timestamps.  Ironically, the AM receives the datapoints
        // exactly as we need  them here, but it then splits them in JsonStatisticsFormatter
        List<VolumeDatapoint> vdps = (entities instanceof List ? (List) entities : new ArrayList<>( entities ));

        // timestamp, map<volname, List<vdp>>>
        // TODO: why do we care if they are ordered?  Influx
        Map<Long, Map<Long, List<VolumeDatapoint>>> orderedVDPs =
                VolumeDatapointHelper.sortByTimestampAndVolumeId( vdps, true );

        final int batchSize = 1024;
        List<Serie> batch = new ArrayList<>(batchSize);

        for ( Map.Entry<Long, Map<Long, List<VolumeDatapoint>>> e : orderedVDPs.entrySet() ) {
            Long ts = e.getKey();
            Map<Long, List<VolumeDatapoint>> volumeDatapoints = e.getValue();

            for ( Map.Entry<Long, List<VolumeDatapoint>> e2 : volumeDatapoints.entrySet() ) {
                // TODO: need volume ids as long everywhere
                Long volid = Long.valueOf( e2.getKey() );
                String volDomain = "";

                // volName is in the list of volume datapoints
                String volName = null;

                metricValues[0] = ts;
                metricValues[1] = volid;
                metricValues[2] = volDomain;

                List<VolumeDatapoint> voldps = e2.getValue();
                for ( VolumeDatapoint vdp : voldps ) {
                    metricValues[3] = vdp.getVolumeName();

                    // find the metric position
                    int midx = indexOf( vdp.getKey() );
                    if (midx == -1) {
                        continue;
                    }
                    metricValues[midx] = vdp.getValue();
                }

//                // update the metric cache
//                metricCache.updateLatestVolumeStats( Long.valueOf( volid ),  voldps );

                Serie serie = new Serie.Builder( VOL_SERIES_NAME )
                        .columns( VOL_METRIC_NAMES.toArray( new String[VOL_METRIC_NAMES.size()] ) )
                        .values(metricValues)
                        .build();

                batch.add( serie );
                if ( batch.size() == batchSize ) {
                    Serie[] writeBatch = batch.toArray( new Serie[ batch.size() ] );
                    batch.clear();

                    // We are now explicitly specifying the timestamp and since VolumeDatapoint timestamps are
                    // in seconds since the epoch, specify a SECONDS precision on the series
                    getConnection().getAsyncDBWriter().write( TimeUnit.SECONDS, writeBatch );
                }
            }
        }

        // if there are any remaining entries in the batch, make sure they are written
        if ( batch.size() > 0 ) {
            Serie[] writeBatch = batch.toArray( new Serie[ batch.size() ] );
            batch.clear();

            // We are now explicitly specifying the timestamp and since VolumeDatapoint timestamps are
            // in seconds since the epoch, specify a SECONDS precision on the series
            getConnection().getAsyncDBWriter().write( TimeUnit.SECONDS, writeBatch );
        }

        return vdps;
    }

}
