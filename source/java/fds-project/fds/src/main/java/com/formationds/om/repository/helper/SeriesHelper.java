/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.builder.DatapointBuilder;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.DoubleStream;

/**
 * @author ptinius
 */
public class SeriesHelper {


    private static final Logger logger =
            LogManager.getLogger( SeriesHelper.class );

    private static final Long SECONDS_IN_HOUR    = TimeUnit.HOURS.toSeconds( 1 );
    private static final Long SECONDS_IN_DAY     = TimeUnit.HOURS.toSeconds( 24 );
    private static final Long SECONDS_IN_WEEK    = TimeUnit.DAYS.toSeconds( 7 );
    private static final Long SECONDS_IN_30_DAYS = TimeUnit.DAYS.toSeconds( 30 );

    // fairly obvious, but... we were passing minutes to the generate method
    // that was previously documented to accept seconds (the implementation
    // of that method was clearly expecting minutes and the doc
    // was out of date).  Using the constants makes it slightly
    // more clear.
    private static final long MINUTES_PER_HOUR      = TimeUnit.HOURS.toMinutes( 1 );
    private static final long MINUTES_PER_SIX_HOURS = TimeUnit.HOURS.toMillis( 6 );
    private static final long MINUTES_PER_DAY       = TimeUnit.DAYS.toMinutes( 1 );

    enum RollupType {
        HOUR,
        DAY,
        WEEK,
        MONTH,
        LONGTERM
    }

    public static RollupType getRollupType(DateRange range) {

        long diff = range.getDuration().getSeconds();
        if (diff <= 0L) {
            throw new IllegalArgumentException( "Date Range is invalid" );
        }

        if( diff <= SECONDS_IN_HOUR ) {
//            logger.trace( "HOUR::{}", diff );
            return RollupType.HOUR;
        } else if( diff <= SECONDS_IN_DAY ) {
//            logger.trace( "DAY::{}", diff );
            return RollupType.DAY;
        } else if( diff <= SECONDS_IN_WEEK ) {
//            logger.trace( "WEEK::{}", diff );
            return RollupType.WEEK;
        } else if( diff <= SECONDS_IN_30_DAYS ) {
//            logger.trace( "30 DAYS::{}", diff );
            return RollupType.MONTH;
        } else if ( diff > 0 && diff > SECONDS_IN_30_DAYS ){
//            logger.trace( "More than 30 days::{}", diff );
            return RollupType.LONGTERM;
        }
        else {
            throw new IllegalArgumentException(
                    "start end timestamp delta is invalid" );
        }
    }

    private final RollupType rollupType;
    private final DateRange dateRange;

    /**
     * default constructor
     */
    SeriesHelper(DateRange dateRange) {
        this.dateRange = dateRange;
        this.rollupType = getRollupType(dateRange);
    }

    /**
     * Generate a roll-up series from the datapoints using the date range from the
     * query and applying the specified statistical operation.
     * <p/>
     * The date range is automatically converted from the source time unit into seconds
     * since the epoch which is what is used for all calculations in the series.
     *
     * @param datapoints the list of source datapoints
     * @param dateRange  the range of datapoints to use in the series
     * @param metrics    the metrics to use in the rollup series (aka "seriesType")
     * @param operation  the operation to apply
     *
     * @return the rollup of the series.  Each Series represents one of the specified metrics.
     */
    public static final List<Series> getRollupSeries( final List<IVolumeDatapoint> datapoints,
                                                      final DateRange dateRange,
                                                      final List<Metrics> metrics,
                                                      final StatOperation operation ) {

        /*
         * So the idea is that we need to sum up all the volume datapoints
         * for a timestamp. Then determine if there enough datapoints to cover
         * what is being asked for.
         *
         * Maximum number of points is 30. Any timestamp that we don't have
         * datapoints for will be set with a datapoint(x=0,y=timestamp).
         *
         * date range diff less than or equal to 'one hours' will provide a
         * datapoint every two minutes, i.e. 30 datapoints.
         *
         * date range diff less than or equal to 'one day' will provide a
         * datapoint every hours, i.e. 24 datapoints
         *
         * date range diff less than or equal to 'one week' will provide a
         * datapoint every 6 hours, i.e. 28 datapoints
         *
         * data range diff less then or equal to '30 days ' will provide a
         * datapoint every 24 hours, i.e. 30 data points
         */
        SeriesHelper sh = new SeriesHelper(dateRange);

        return sh.doRollup(datapoints, metrics, operation);
    }

    private List<Series> doRollup( final List<IVolumeDatapoint> datapoints,
                                   final List<Metrics> metrics,
                                   final StatOperation operation) {
        long startTimeEpochSeconds = dateRange.getStart();

        switch (rollupType) {
            case HOUR:
                return hourRollup(datapoints, startTimeEpochSeconds, metrics, operation);
            case DAY:
                return dayRollup(datapoints, startTimeEpochSeconds, metrics, operation);
            case WEEK:
                return weekRollup(datapoints, startTimeEpochSeconds, metrics, operation);
            case MONTH:
                return thirtyDaysRollup(datapoints, startTimeEpochSeconds, metrics, operation);
            case LONGTERM:
                return longTermRollup(datapoints, startTimeEpochSeconds, metrics, operation);
            default:
                throw new IllegalArgumentException("Unknown rollup type");
        }
    }

    private List<Series> hourRollup( final List<IVolumeDatapoint> datapoints,
                                            final Long startTimeEpochSeconds,
                                            final List<Metrics> metrics,
                                            final StatOperation operation ) {

        final List<Series> series = new ArrayList<Series>();

        /*
         * for each volume
         *  sum all datapoints for each 2 minute interval to produce one
         *  datapoint, per volume per 2 minute interval should be 30
         *  data points
         */
        for ( Metrics metric : metrics ) {

            series.add( generate( datapoints, startTimeEpochSeconds, metric, 2L, 30, operation ) );
        }

        return series;
    }

    private List<Series> dayRollup(	final List<IVolumeDatapoint> datapoints,
                                          	final Long epochSeconds,
                                          	final List<Metrics> metrics,
                                          	final StatOperation operation	) {

        final List<Series> series = new ArrayList<Series>();

        /*
         * for each volume
         *  sum all datapoints for each hour interval to produce one
         *  datapoint, per volume per hour interval should be 30
         *  data points
         */
        for ( Metrics metric : metrics ) {
            series.add( generate( datapoints, epochSeconds, metric, MINUTES_PER_HOUR, 30, operation ) );
        }

        return series;
    }

    private List<Series> weekRollup(final List<IVolumeDatapoint> datapoints,
                                           final Long epochSeconds,
                                           final List<Metrics> metrics,
                                           final StatOperation operation ) {

        final List<Series> series = new ArrayList<Series>();

        /*
         * for each volume
         *  sum all datapoints for each 6 hour interval to produce one
         *  datapoint, per volume per 6 hour interval should be 30
         *  data points
         */
        for ( Metrics metric : metrics ) {
            series.add( generate( datapoints, epochSeconds, metric, MINUTES_PER_SIX_HOURS, 28, operation ) );
        }

        return series;
    }

    private List<Series> thirtyDaysRollup(	final List<IVolumeDatapoint> datapoints,
                                                 	final Long epochSeconds,
                                                 	final List<Metrics> metrics,
                                                 	final StatOperation operation ) {

        final List<Series> series = new ArrayList<Series>();

        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */
        for ( Metrics metric : metrics ) {
            series.add( generate( datapoints, epochSeconds, metric, MINUTES_PER_DAY, 30, operation ) );
        }

        return series;
    }

    private List<Series> longTermRollup( final List<IVolumeDatapoint> datapoints,
                                                final long epochSeconds,
                                                final List<Metrics> metrics,
                                                final StatOperation operation ){

        final List<Series> series = new ArrayList<Series>();

        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */
        for ( Metrics metric : metrics ) {
            // just setting it to one point per 2 days
            series.add( generate( datapoints, epochSeconds, metric, 2*MINUTES_PER_DAY, 999, operation ) );
        }

        return series;
    }

    /**
     * This method will take the list and parse it down to only
     * values that match the metric argument.  Then it will
     * sum these values into buckets defined by the distribution value (in seconds).
     *
     * If requested (operation = RATE), it will transform those buckets into rates based
     * on the distribution.
     *
     * It will then map these into a series object ready for display in a chart
     * or other statistical display mechanisms
     *
     * @param volumeDatapoints the list of datapoints to generate the series from
     * @param startTimestampEpochSeconds timestamp in seconds since the epoch
     * @param metrics the metric to generate the series on
     * @param distributionMinutes the distribution of the series in minutes
     * @param maxResults the max number of results to include in the series
     * @param operation the stat operation to apply
     *
     * @return the generated series
     */
    protected static Series generate( final List<IVolumeDatapoint> volumeDatapoints,
                                      final Long startTimestampEpochSeconds,
                                      final Metrics metrics,
                                      final Long distributionMinutes,
                                      final int maxResults,
                                      final StatOperation operation ) {

        Map<Long, Set<? extends IVolumeDatapoint>> groupByTimestamp =
                VolumeDatapointHelper.groupByTimestamp( volumeDatapoints );

        final List<Datapoint> datapoints = new ArrayList<>( );
        groupByTimestamp.forEach( ( bytesTimestamp, bytesValues ) -> {

            // this is always a sum because it represents collapsing
            // a set of points that are cumulative numbers by nature because there is one per volume
            // i.e. if you are wanting the average number of PUTS over a time frame
            // you still need to sum them here, and divide that number by the
            // distribution in order to get a real average.  Otherwise you'll
            // be averaging an average which will be significantly lower than
            // the value you desire.
            Double d = bytesValues.stream()
                    .filter( ( value ) -> {
                        return metrics.matches( value.getKey() );
                    })
//                    .peek( ( l ) -> logger.trace( l.toString() ) )
                    .mapToDouble( IVolumeDatapoint::getValue )
                    .sum();

//            logger.trace( "DOUBLE::{} LONG::{} TIMESTAMP::{}",
//                          d, d.longValue(), bytesTimestamp );

            datapoints.add( new DatapointBuilder().withY( d )
                            .withX( (double)bytesTimestamp )
                            .build() );
        } );

//        logger.trace( "START::{} INTERVAL::{} MAX::{} SIZE::{}",
//                      startTimestampEpochSeconds, distributionMinutes, maxResults, datapoints.size() );

        final List<Datapoint> results = new ArrayList<>( );

        // we need to create a bucket for each "distribution" size counting back from
        // the start time maxResults number of times.
        Map<Double, List<Datapoint>> bucketMap = new HashMap<Double, List<Datapoint>>();

        for ( Datapoint dp : datapoints ){

            // normalize the key so it's the value of one of our buckets
            // which are separated by "distribution" and start at "timestamp"
            Double diff = dp.getX() - startTimestampEpochSeconds;

            Double bucket = Math.floor( diff /
                                        ( double ) TimeUnit.MINUTES.toSeconds( distributionMinutes ) );
            bucket = startTimestampEpochSeconds + ( bucket * TimeUnit.MINUTES.toSeconds( distributionMinutes ) );

            List<Datapoint> bucketList = bucketMap.get( bucket );

            if ( bucketList == null ) {
                bucketList = new ArrayList<>();
            }

            bucketList.add( dp );
            bucketMap.put( bucket,  bucketList );
        }

        for ( Double key : bucketMap.keySet() ){

            Double rolledupValue = 0.0;
            DoubleStream dsY = bucketMap.get( key ).stream().mapToDouble( Datapoint::getY );

            switch( operation ) {
                case RATE:
                    rolledupValue = dsY.sum() / TimeUnit.MINUTES.toSeconds( distributionMinutes );
                    break;
                case MAX_Y:
                    // capacity is an example of using max for bucket calculation
                    // each time capacity is reported, it's the current state of the system
                    // so summing each value in this list makes no sense - so we take the last
                    // value (or max) in the list and use it for our indicator of the
                    // state of the system at this time.
                    rolledupValue = dsY.max().getAsDouble();
                    break;
                case MAX_X:
                    // getting the max datapoint by time (i.e. most recent) but use the Y value
                    Optional<Datapoint> maxDatapoint = bucketMap.get( key ).stream().max( (dp1, dp2) ->{
                        return dp1.getX().compareTo( dp2.getX() );
                    });

                    if ( maxDatapoint.isPresent() ){
                        rolledupValue = maxDatapoint.get().getY();
                    }

                    break;
                case SUM:
                    // the default case is to sum everything in this bucket assuming the values
                    // are related only to activity within the time that this bucket
                    // encapsulates.  Performance # of gets is a good example of why you would use
                    // SUM
                default:
                    rolledupValue = dsY.sum();
                    break;
            }

            results.add( new DatapointBuilder().withX( key ).withY( rolledupValue ).build() );
        }

        results.sort( ( dp1, dp2 ) -> dp1.getX().compareTo( dp2.getX() ) );

        if ( results.isEmpty() ){

            // at start time
            results.add( new DatapointBuilder()
                         .withX( (double) startTimestampEpochSeconds )
                         .withY( 0.0 ).build() );

            // at end time
            results.add( new DatapointBuilder()
                         .withX( (double) startTimestampEpochSeconds +
                                 (maxResults * TimeUnit.MINUTES.toSeconds( distributionMinutes ) ) )
                         .withY( 0.0 ).build() );
        }

        // if our earliest timestamp is after the requested start time we will add a zero "distribution"
        // before the earliest, and a zero at the requested start time.
        else if ( results.get( 0 ).getX() > startTimestampEpochSeconds ){

            // a point just earlier than the first real point. ... let's do one second
            results.add( 0, new DatapointBuilder()
                         .withX( results.get( 0 ).getX() - 1 )
                         .withY( 0.0 ).build() );

            // at start time
            results.add( 0, new DatapointBuilder()
                         .withX( (double) startTimestampEpochSeconds )
                         .withY( 0.0 ).build() );
        }

        return new SeriesBuilder().withType( metrics )
                .withDatapoints( results )
                .build();
    }
}
