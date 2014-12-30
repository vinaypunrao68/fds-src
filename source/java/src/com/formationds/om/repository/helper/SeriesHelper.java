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
import com.formationds.om.repository.query.MetricQueryCriteria;
//import com.formationds.om.repository.query.QueryCriteria;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public class SeriesHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( SeriesHelper.class );

    private static final Long SECONDS_IN_HOUR = TimeUnit.HOURS.toSeconds( 1 );
    private static final Long SECONDS_IN_DAY = TimeUnit.HOURS.toSeconds( 24 );
    private static final Long SECONDS_IN_WEEK = TimeUnit.DAYS.toSeconds( 7 );
    private static final Long SECONDS_IN_30_DAYS = TimeUnit.DAYS.toSeconds( 30 );

    private final DatapointHelper dpHelper;
    private final VolumeDatapointHelper vdpHelper;

    /**
     * default constructor
     */
    public SeriesHelper() {
        this.dpHelper = new DatapointHelper();
        this.vdpHelper = new VolumeDatapointHelper();
    }

    public final List<Series> getSummedSeries(
    	final List<VolumeDatapoint> datapoints,
    	final MetricQueryCriteria query ) {
    	
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
        long epoch = 0L;
        if( query.getRange() != null ) {
            final DateRange dateRange = query.getRange();
            if( dateRange.getStart() != null && dateRange.getEnd() != null ) {

                diff = dateRange.getEnd() - dateRange.getStart();
                epoch = dateRange.getStart();

            } else if( dateRange.getStart() != null ) {

                diff = DateTimeUtil.toUnixEpoch( LocalDateTime.now() ) -
                       dateRange.getStart();
                epoch = dateRange.getStart();

            } else if( dateRange.getEnd() != null ) {

                epoch = DateTimeUtil.toUnixEpoch( LocalDateTime.now() );
                diff = dateRange.getEnd() - epoch;
            }
        }

        if( diff > 0L ) {
            if( diff <= SECONDS_IN_HOUR ) {
                logger.trace( "HOUR::{}", diff );
                return hourRollup( datapoints, epoch, query.getSeriesType() );
            } else if( diff <= SECONDS_IN_DAY ) {
                logger.trace( "DAY::{}", diff );
                return dayRollup( datapoints, epoch, query.getSeriesType() );
            } else if( diff <= SECONDS_IN_WEEK ) {
                logger.trace( "WEEK::{}", diff );
                return weekRollup( datapoints, epoch, query.getSeriesType() );
            } else if( diff <= SECONDS_IN_30_DAYS ) {
                logger.trace( "30 DAYS::{}", diff );
                return thirtyDaysRollup( datapoints, epoch, query.getSeriesType() );
            } else {
                throw new IllegalArgumentException(
                    "start end timestamp delta is invalid" );
            }
        }

        throw new IllegalArgumentException( "Date Range is invalid" );    	
    }
    
    private List<Series> hourRollup( final List<VolumeDatapoint> datapoints, 
    								 final Long epoch,
    								 final List<Metrics> metrics ) {
    	
    	final List<Series> series = new ArrayList<Series>();
    	
        /*
         * for each volume
         *  sum all datapoints for each 2 minute interval to produce one
         *  datapoint, per volume per 2 minute interval should be 30
         *  data points
         */
    	for ( Metrics metric : metrics ) {
    		
    		series.add( generate( datapoints, epoch, metric, 2L, 30 ) );
    	}
    	
    	return series;
    }
    
    private List<Series> dayRollup(final List<VolumeDatapoint> datapoints, 
			 						final Long epoch,
			 							final List<Metrics> metrics ) {

    	final List<Series> series = new ArrayList<Series>();
	
        /*
         * for each volume
         *  sum all datapoints for each hour interval to produce one
         *  datapoint, per volume per hour interval should be 30
         *  data points
         */
		for ( Metrics metric : metrics ) {
			series.add( generate( datapoints, epoch, metric, 60L, 30 ) );
		}
	
		return series;
	}    
    
    private List<Series> weekRollup(final List<VolumeDatapoint> datapoints, 
									final Long epoch,
									final List<Metrics> metrics ) {

		final List<Series> series = new ArrayList<Series>();
		
        /*
         * for each volume
         *  sum all datapoints for each 6 hour interval to produce one
         *  datapoint, per volume per 6 hour interval should be 30
         *  data points
         */
		for ( Metrics metric : metrics ) {
			series.add( generate( datapoints, epoch, metric, 360L, 28 ) );
		}
		
		return series;
	}      
    
    private List<Series> thirtyDaysRollup(	final List<VolumeDatapoint> datapoints, 
											final Long epoch,
											final List<Metrics> metrics ) {

		final List<Series> series = new ArrayList<Series>();
		
        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */
		for ( Metrics metric : metrics ) {
			series.add( generate( datapoints, epoch, metric, 1440L, 30 ) );
		}
		
		return series;
	}  
    
//    /**
//     * @param datapoints the {@link java.util.List} of {@link com.formationds.commons.model.entity.VolumeDatapoint}
//     * @param query the {@link com.formationds.om.repository.query.QueryCriteria}
//     *
//     * @return Returns the {@link java.util.List} of {@link com.formationds.commons.model.Series}
//     */
//    public final List<Series> getCapacitySeries(
//        final List<VolumeDatapoint> datapoints,
//        final QueryCriteria query ) {
//
//        /*
//         * So the idea is that we need to sum up all the volume datapoints
//         * for a timestamp. Then determine if there enough datapoints to cover
//         * what is being asked for.
//         *
//         * Maximum number of points is 30. Any timestamp that we don't
//         * datapoints for will be set with a datapoint(x=0,y=timestamp).
//         *
//         * date range diff less than or equal to 'one hours' will provide a
//         * datapoint every two minutes, i.e. 30 datapoints.
//         *
//         * date range diff less than or equal to 'one day' will provide a
//         * datapoint every hours, i.e. 24 datapoints
//         *
//         * date range diff less than or equal to 'one week' will provide a
//         * datapoint every 6 hours, i.e. 28 datapoints
//         *
//         * data range diff less then or equal to '30 days ' will provide a
//         * datapoint every 24 hours, i.e. 30 data points
//         */
//        long diff = 0L;
//        long epoch = 0L;
//        if( query.getRange() != null ) {
//            final DateRange dateRange = query.getRange();
//            if( dateRange.getStart() != null && dateRange.getEnd() != null ) {
//
//                diff = dateRange.getEnd() - dateRange.getStart();
//                epoch = dateRange.getStart();
//
//            } else if( dateRange.getStart() != null ) {
//
//                diff = DateTimeUtil.toUnixEpoch( LocalDateTime.now() ) -
//                       dateRange.getStart();
//                epoch = dateRange.getStart();
//
//            } else if( dateRange.getEnd() != null ) {
//
//                epoch = DateTimeUtil.toUnixEpoch( LocalDateTime.now() );
//                diff = dateRange.getEnd() - epoch;
//            }
//        }
//
//        if( diff > 0L ) {
//            if( diff <= SECONDS_IN_HOUR ) {
//                logger.trace( "HOUR::{}", diff );
//                return hourCapacity( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_DAY ) {
//                logger.trace( "DAY::{}", diff );
//                return dayCapacity( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_WEEK ) {
//                logger.trace( "WEEK::{}", diff );
//                return weekCapacity( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_30_DAYS ) {
//                logger.trace( "30 DAYS::{}", diff );
//                return thirtyDaysCapacity( datapoints, epoch );
//            } else {
//                throw new IllegalArgumentException(
//                    "start end timestamp delta is invalid" );
//            }
//        }
//
//        throw new IllegalArgumentException( "Date Range is invalid" );
//    }
//
//    private List<Series> hourCapacity( final List<VolumeDatapoint> datapoints,
//                                       final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//        /*
//         * for each volume
//         *  sum all datapoints for each 2 minute interval to produce one
//         *  datapoint, per volume per 2 minute interval should be 30
//         *  data points
//         */
//
//        series.add( generate( datapoints, epoch, Metrics.LBYTES, 2L, 30 ) );
//        series.add( generate( datapoints, epoch, Metrics.PBYTES, 2L, 30 ) );
//
//        return series;
//    }
//
//    private List<Series> dayCapacity( final List<VolumeDatapoint> datapoints,
//                                      final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each hours to produce one datapoint,
//         *  per volume
//         */
//
//        // every six hours
//        series.add( generate( datapoints, epoch, Metrics.LBYTES, 60L, 24 ) );
//        series.add( generate( datapoints, epoch, Metrics.PBYTES, 60L, 24 ) );
//
//        return series;
//    }
//
//    private List<Series> weekCapacity( final List<VolumeDatapoint> datapoints,
//                                       final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each 6 hours to produce one datapoint,
//         *  per volume
//         */
//        series.add( generate( datapoints, epoch, Metrics.LBYTES, 360L, 28 ) );
//        series.add( generate( datapoints, epoch, Metrics.PBYTES, 360L, 28 ) );
//
//        return series;
//    }
//
//    private List<Series> thirtyDaysCapacity(
//        final List<VolumeDatapoint> datapoints,
//        final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each day to produce one datapoint,
//         *  per volume
//         */
//
//        // every 1 day
//        series.add( generate( datapoints, epoch, Metrics.LBYTES, 1440L, 30 ) );
//        series.add( generate( datapoints, epoch, Metrics.PBYTES, 1440L, 30 ) );
//
//        return series;
//    }
//
//    /**
//     * @param datapoints the {@link java.util.List} of {@link com.formationds.commons.model.entity.VolumeDatapoint}
//     * @param query the {@link com.formationds.om.repository.query.QueryCriteria}
//     *
//     * @return Returns the {@link java.util.List} of {@link com.formationds.commons.model.Series}
//     */
//    public final List<Series> getPerformanceSeries(
//        final List<VolumeDatapoint> datapoints,
//        final QueryCriteria query ) {
//
//        /*
//         * So the idea is that we need to sum up all the volume datapoints
//         * for a timestamp. Then determine if there enough datapoints to cover
//         * what is being asked for.
//         *
//         * Maximum number of points is 30. Any timestamp that we don't
//         * datapoints for will be set with a datapoint(x=0,y=timestamp).
//         *
//         * date range diff less than or equal to 'one hours' will provide a
//         * datapoint every two minutes, i.e. 30 datapoints.
//         *
//         * date range diff less than or equal to 'one day' will provide a
//         * datapoint every hours, i.e. 24 datapoints
//         *
//         * date range diff less than or equal to 'one week' will provide a
//         * datapoint every 6 hours, i.e. 28 datapoints
//         *
//         * data range diff less then or equal to '30 days ' will provide a
//         * datapoint every 24 hours, i.e. 30 data points
//         */
//        long diff = 0L;
//        long epoch = 0L;
//        if( query.getRange() != null ) {
//            final DateRange dateRange = query.getRange();
//            if( dateRange.getStart() != null && dateRange.getEnd() != null ) {
//
//                diff = dateRange.getEnd() - dateRange.getStart();
//                epoch = dateRange.getStart();
//
//            } else if( dateRange.getStart() != null ) {
//
//                diff = DateTimeUtil.toUnixEpoch( LocalDateTime.now() ) -
//                    dateRange.getStart();
//                epoch = dateRange.getStart();
//
//            } else if( dateRange.getEnd() != null ) {
//
//                epoch = DateTimeUtil.toUnixEpoch( LocalDateTime.now() );
//                diff = dateRange.getEnd() - epoch;
//            }
//        }
//
//        if( diff > 0L ) {
//            if( diff <= SECONDS_IN_HOUR ) {
//                return hourPerformance( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_DAY ) {
//                return dayPerformance( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_WEEK ) {
//                return weekPerformance( datapoints, epoch );
//            } else if( diff <= SECONDS_IN_30_DAYS ) {
//                return thirtyDaysPerformance( datapoints, epoch );
//            } else {
//                throw new IllegalArgumentException(
//                    "start end timestamp delta is invalid" );
//            }
//        }
//
//        throw new IllegalArgumentException( "Date Range is invalid" );
//    }
//
//    private List<Series> hourPerformance(
//        final List<VolumeDatapoint> datapoints,
//        final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each 2 minute interval to produce one
//         *  datapoint, per volume
//         */
//
//        series.add( generate( datapoints, epoch, Metrics.STP_WMA, 2L, 30 ) );
//
//        return series;
//    }
//
//    private List<Series> dayPerformance(
//        final List<VolumeDatapoint> datapoints,
//        final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each hours to produce one datapoint,
//         *  per volume
//         */
//        series.add( generate( datapoints, epoch, Metrics.STP_WMA, 60L, 24 ) );
//
//        return series;
//    }
//
//    private List<Series> weekPerformance(
//        final List<VolumeDatapoint> datapoints,
//        final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each 6 hours to produce one datapoint,
//         *  per volume
//         */
//        series.add( generate( datapoints, epoch, Metrics.STP_WMA, 360L, 28 ) );
//
//        return series;
//    }
//
//    private List<Series> thirtyDaysPerformance(
//        final List<VolumeDatapoint> datapoints,
//        final Long epoch ) {
//
//        final List<Series> series = new ArrayList<>( );
//
//        /*
//         * for each volume
//         *  sum all datapoints for each day to produce one datapoint,
//         *  per volume
//         */
//        series.add( generate( datapoints, epoch, Metrics.STP_WMA, 1440L, 30 ) );
//
//        return series;
//    }

    private Series generate(
        final List<VolumeDatapoint> volumeDatapoints,
        final Long timestamp,
        final Metrics metrics,
        final Long distribution,
        final int maxResults ) {

        Map<Long, Set<VolumeDatapoint>> groupByTimestamp =
            vdpHelper.groupByTimestamp( volumeDatapoints );

        final List<Datapoint> datapoints = new ArrayList<>( );
        groupByTimestamp.forEach( ( bytesTimestamp, bytesValues ) -> {
            final Double d = bytesValues.stream()
                                        .filter( ( value ) ->
                                                     value.getKey()
                                                          .equalsIgnoreCase(
                                                              metrics.key() ) )
                                        .peek( ( l ) -> logger.trace( l.toString() ) )
                                        .mapToDouble(
                                            VolumeDatapoint::getValue )
                                        .sum();
            logger.trace( "DOUBLE::{} LONG::{} TIMESTAMP::{}",
                          d, d.longValue(), bytesTimestamp );

            datapoints.add( new DatapointBuilder().withY( d.longValue() )
                                                  .withX( bytesTimestamp )
                                                  .build() );
        } );

        logger.trace( "START::{} INTERVAL::{} MAX::{} SIZE::{}",
                      timestamp, distribution, maxResults, datapoints.size() );

        final List<Datapoint> results = new ArrayList<>( );
        if( datapoints.size() < maxResults ) {
            // to few datapoints, need to pad with empty datapoints
            final int needed = maxResults - datapoints.size();
            dpHelper.padWithEmptyDatapoints( timestamp,
                                             TimeUnit.MINUTES.toSeconds( distribution ),
                                             needed,
                                             datapoints );

            results.addAll( datapoints );
        } else if( datapoints.size() > maxResults ) {

            /*
             * TODO finish implementation
             *
             * need to get a even distribution of datapoints with a maximum of maxResults
             */

            results.addAll( datapoints.subList( datapoints.size() - maxResults,
                                                datapoints.size() ) );
        } else {
            results.addAll( datapoints );
        }

        return new SeriesBuilder().withType( metrics )
                                  .withDatapoints( results )
                                  .build();
    }
}
