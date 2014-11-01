/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.builder.DatapointBuilder;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.om.repository.query.QueryCriteria;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class SeriesHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( SeriesHelper.class );

    private static final Integer SECOND_IN_HOUR = 60 * 60;
    private static final Integer SECONDS_IN_DAY = SECOND_IN_HOUR * 24;
    private static final Integer SECONDS_IN_WEEK = SECONDS_IN_DAY * 7;
    private static final Integer SECONDS_IN_30_DAYS = SECONDS_IN_DAY * 30;

    /**
     * @param datapoints
     * @param query
     *
     * @return
     */
    public final List<Series> getCapacitySeries(
        final List<VolumeDatapoint> datapoints,
        final QueryCriteria query ) {

        /*
         * So the idea is that we need to sum up all the volume datapoints
         * for a timestamp. Then determine if there enough datapoints to cover
         * what is being asked for.
         *
         * Maximum number of points is 30. Any timestamp that we don't
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
        long diff = 0L;
        long epochStart = 0L;
        if( query.getRange() != null ) {
            final DateRange dateRange = query.getRange();
            if( dateRange.getStart() != null && dateRange.getEnd() != null ) {

                diff = dateRange.getEnd() - dateRange.getStart();
                epochStart = dateRange.getStart();

            } else if( dateRange.getStart() != null ) {

                diff = DateTimeUtil.toUnixEpoch( LocalDateTime.now() ) -
                       dateRange.getStart();
                epochStart = dateRange.getStart();

            } else if( dateRange.getEnd() != null ) {

                epochStart = DateTimeUtil.toUnixEpoch( LocalDateTime.now() );
                diff = dateRange.getEnd() -
                       epochStart;
            }
        }

        if( diff > 0L ) {
            if( diff <= SECOND_IN_HOUR ) {
                return hourCapacity( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_DAY ) {
                return dayCapacity( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_WEEK ) {
                return weekCapacity( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_30_DAYS ) {
                return thirtyDaysCapacity( datapoints, epochStart );
            } else {
                throw new IllegalArgumentException(
                    "start end timestamp delta is invalid" );
            }
        }

        throw new IllegalArgumentException( "Date Range is invalid" );
    }

    private List<Series> hourCapacity( final List<VolumeDatapoint> datapoints,
                                       final long epochStart ) {

        final List<Series> series = new ArrayList<>( );
        /*
         * for each volume
         *  sum all datapoints for each 2 minute interval to produce one
         *  datapoint, per volume per 2 minute interval should be 30
         *  data points
         */

        series.add( capacity( datapoints, epochStart, Metrics.LBYTES, 2, 30 ) );
        series.add( capacity( datapoints, epochStart, Metrics.PBYTES, 2, 30 ) );

//        Map<Long, List<VolumeDatapoint>> groupByTimestamp =
//            groupByTimestamp( datapoints );
//
//        final List<Datapoint> lbytes =
//            padWithEmptyDatapoints( epochStart,
//                                    TimeUnit.MINUTES.toSeconds( 2 ),
//                                    30 );
//
//        groupByTimestamp.forEach( ( lbytesTimestamp, lbytesValues ) -> {
//            final Double d = lbytesValues.stream()
//                                         .filter( ( value ) ->
//                                                      value.getKey()
//                                                           .equalsIgnoreCase(
//                                                               Metrics.LBYTES.key() ) )
//                                         .mapToDouble( VolumeDatapoint::getValue )
//                                         .sum();
//
//            lbytes.add( new DatapointBuilder().withY( d.longValue() )
//                                              .withX( lbytesTimestamp )
//                                              .build() );
//        } );

//        final int lbytes_to = lbytes.size();
//        final int lbytes_from = lbytes_to - 30;
//        series.add( new SeriesBuilder().withType( Metrics.LBYTES )
//                                       .withDatapoints(
//                                           lbytes.subList( lbytes_from,
//                                                           lbytes_to ) )
//                                       .build() );

//        final List<Datapoint> pbytes =
//            padWithEmptyDatapoints( epochStart,
//                                    TimeUnit.MINUTES.toSeconds( 2 ),
//                                    30 );
//
//        groupByTimestamp.forEach( ( pbytesTimestamp, pbytesValues ) -> {
//            final Double d = pbytesValues.stream()
//                                         .filter( ( value ) ->
//                                                      value.getKey()
//                                                           .equalsIgnoreCase(
//                                                               Metrics.PBYTES.key() ) )
//                                         .mapToDouble( VolumeDatapoint::getValue )
//                                         .sum();
//
//            pbytes.add( new DatapointBuilder().withY( d.longValue() )
//                                             .withX( pbytesTimestamp )
//                                              .build() );
//        } );
//
//        final int pbytes_to = pbytes.size();
//        final int pbytes_from = pbytes_to - 30;
//        series.add(
//            new SeriesBuilder().withType( Metrics.PBYTES )
//                               .withDatapoints( pbytes.subList( pbytes_from,
//                                                                pbytes_to ) )
//                               .build() );

        return series;
    }

    private List<Series> dayCapacity( final List<VolumeDatapoint> datapoints,
                                      final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each  hours to produce one datapoint,
         *  per volume
         */

        // every six hours
        series.add( capacity( datapoints, epochStart, Metrics.LBYTES, 60, 24 ) );
        series.add( capacity( datapoints, epochStart, Metrics.PBYTES, 60, 24 ) );

        return series;
    }

    private List<Series> weekCapacity( final List<VolumeDatapoint> datapoints,
                                       final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each 6 hours to produce one datapoint,
         *  per volume
         */
        series.add( capacity( datapoints, epochStart, Metrics.LBYTES, 360, 28 ) );
        series.add( capacity( datapoints, epochStart, Metrics.PBYTES, 360, 28 ) );

        return series;
    }

    private List<Series> thirtyDaysCapacity(
        final List<VolumeDatapoint> datapoints,
        final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */

        // every 1 day
        series.add( capacity( datapoints, epochStart, Metrics.LBYTES, 1440, 30 ) );
        series.add( capacity( datapoints, epochStart, Metrics.PBYTES, 1440, 30 ) );

        return series;
    }

    private Series capacity(
        final List<VolumeDatapoint> volumeDatapoints,
        final long epoch,
        final Metrics metrics,
        final int interval,
        final int maxResults ) {

        final List<Datapoint> datapoints =
            padWithEmptyDatapoints( epoch,
                                    TimeUnit.MINUTES.toSeconds( interval ),
                                    maxResults );

        Map<Long, List<VolumeDatapoint>> groupByTimestamp =
            groupByTimestamp( volumeDatapoints );
        groupByTimestamp.forEach( ( pbytesTimestamp, pbytesValues ) -> {
            final Double d = pbytesValues.stream()
                                         .filter( ( value ) ->
                                                      value.getKey()
                                                           .equalsIgnoreCase(
                                                               metrics.key() ) )
                                         .mapToDouble( VolumeDatapoint::getValue )
                                         .sum();

            datapoints.add( new DatapointBuilder().withY( d.longValue() )
                                                  .withX( pbytesTimestamp )
                                                  .build() );
        } );

        final int to = datapoints.size();
        final int from = to - maxResults;

        return new SeriesBuilder().withType( metrics )
                                  .withDatapoints(
                                      datapoints.subList( from,
                                                          to ) )
                                  .build();
    }

    public final List<Series> getPerformanceSeries(
        final List<VolumeDatapoint> datapoints,
        final QueryCriteria query ) {

        /*
         * So the idea is that we need to sum up all the volume datapoints
         * for a timestamp. Then determine if there enough datapoints to cover
         * what is being asked for.
         *
         * Maximum number of points is 30. Any timestamp that we don't
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
        long diff = 0L;
        long epochStart = 0L;
        if( query.getRange() != null ) {
            final DateRange dateRange = query.getRange();
            if( dateRange.getStart() != null && dateRange.getEnd() != null ) {

                diff = dateRange.getEnd() - dateRange.getStart();
                epochStart = dateRange.getStart();

            } else if( dateRange.getStart() != null ) {

                diff = DateTimeUtil.toUnixEpoch( LocalDateTime.now() ) -
                    dateRange.getStart();
                epochStart = dateRange.getStart();

            } else if( dateRange.getEnd() != null ) {

                epochStart = DateTimeUtil.toUnixEpoch( LocalDateTime.now() );
                diff = dateRange.getEnd() -
                    epochStart;
            }
        }

        if( diff > 0L ) {
            if( diff <= SECOND_IN_HOUR ) {
                return hourPerformance( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_DAY ) {
                return dayPerformance( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_WEEK ) {
                return weekPerformance( datapoints, epochStart );
            } else if( diff <= SECONDS_IN_30_DAYS ) {
                return thirtyDaysPerformance( datapoints, epochStart );
            } else {
                throw new IllegalArgumentException(
                    "start end timestamp delta is invalid" );
            }
        }

        throw new IllegalArgumentException( "Date Range is invalid" );
    }

    private List<Series> hourPerformance(
        final List<VolumeDatapoint> datapoints,
        final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each 2 minute interval to produce one
         *  datapoint, per volume
         */

        series.add( capacity( datapoints, epochStart, Metrics.STP_WMA, 2, 30 ) );
        series.add( capacity( datapoints, epochStart, Metrics.LTP_WMA, 2, 30 ) );

        return series;
    }

    private List<Series> dayPerformance(
        final List<VolumeDatapoint> datapoints,
        final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each  hours to produce one datapoint,
         *  per volume
         */
        series.add( capacity( datapoints, epochStart, Metrics.STP_WMA, 60, 24 ) );
        series.add( capacity( datapoints, epochStart, Metrics.LTP_WMA, 60, 24 ) );

        return series;
    }

    private List<Series> weekPerformance(
        final List<VolumeDatapoint> datapoints,
        final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each 6 hours to produce one datapoint,
         *  per volume
         */
        series.add( performance( datapoints, epochStart, Metrics.STP_WMA, 360, 28 ) );
        series.add( performance( datapoints, epochStart, Metrics.LTP_WMA, 360, 28 ) );

        return series;
    }

    private List<Series> thirtyDaysPerformance(
        final List<VolumeDatapoint> datapoints,
        final long epochStart ) {

        final List<Series> series = new ArrayList<>( );

        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */
        series.add( performance( datapoints, epochStart, Metrics.STP_WMA, 1440, 30 ) );
        series.add( performance( datapoints, epochStart, Metrics.LTP_WMA, 1440, 30 ) );

        return series;
    }

    private Series performance(
        final List<VolumeDatapoint> volumeDatapoints,
        final long epoch,
        final Metrics metrics,
        final int interval,
        final int maxResults ) {

        final List<Datapoint> datapoints =
            padWithEmptyDatapoints( epoch,
                                    TimeUnit.MINUTES.toSeconds( interval ),
                                    maxResults );

        Map<Long, List<VolumeDatapoint>> groupByTimestamp =
            groupByTimestamp( volumeDatapoints );
        groupByTimestamp.forEach( ( pbytesTimestamp, pbytesValues ) -> {
            final Double d = pbytesValues.stream()
                                         .filter( ( value ) ->
                                                      value.getKey()
                                                           .equalsIgnoreCase(
                                                               metrics.key() ) )
                                         .mapToDouble( VolumeDatapoint::getValue )
                                         .sum();

            datapoints.add( new DatapointBuilder().withY( d.longValue() )
                                                  .withX( pbytesTimestamp )
                                                  .build() );
        } );

        final int to = datapoints.size();
        final int from = to - maxResults;

        return new SeriesBuilder().withType( metrics )
                                  .withDatapoints(
                                      datapoints.subList( from,
                                                          to ) )
                                  .build();
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link Map} of {@code timestamp} and {@link List} of datapoint
     */
    private Map<Long, List<VolumeDatapoint>> groupByTimestamp(
        final List<VolumeDatapoint> datapoints ) {

        return datapoints.stream()
                         .collect(
                             Collectors.groupingBy(
                                 VolumeDatapoint::getTimestamp ) );
    }

    /**
     * @param start the {@code long} representing the starting date range
     * @param interval the {@link long} representing the interval at which the
     *                 datapoints are created
     * @param count the {@code count} representing the number of datapoints
     *
     * @return Returns {@link List} of {@link com.formationds.commons.model.Datapoint}
     */
    private List<Datapoint> padWithEmptyDatapoints( final long start,
                                                    final long interval,
                                                    final int count ) {
        final List<Datapoint> empty = new ArrayList<>( );

        long epoch = start;
        for( int i = 0; i < count; i++ ) {
            empty.add( new DatapointBuilder().withX( epoch )
                                             .withY( 0L )
                                             .build() );
            epoch = epoch + interval;
        }

        return empty;
    }

}
