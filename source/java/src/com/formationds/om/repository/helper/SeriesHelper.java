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
import com.formationds.commons.model.type.StatOperation;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.om.repository.query.MetricQueryCriteria;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.DoubleStream;

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

    public final List<Series> getRollupSeries(
    	final List<VolumeDatapoint> datapoints,
    	final MetricQueryCriteria query,
    	final StatOperation operation ) {
    	
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
                return hourRollup( datapoints, epoch, query.getSeriesType(), operation );
            } else if( diff <= SECONDS_IN_DAY ) {
                logger.trace( "DAY::{}", diff );
                return dayRollup( datapoints, epoch, query.getSeriesType(), operation );
            } else if( diff <= SECONDS_IN_WEEK ) {
                logger.trace( "WEEK::{}", diff );
                return weekRollup( datapoints, epoch, query.getSeriesType(), operation );
            } else if( diff <= SECONDS_IN_30_DAYS ) {
                logger.trace( "30 DAYS::{}", diff );
                return thirtyDaysRollup( datapoints, epoch, query.getSeriesType(), operation );
            } else if ( diff > 0 && diff > SECONDS_IN_30_DAYS ){
            	logger.trace( "More than 30 days::{}", diff );
            	return longTermRollup( datapoints, epoch, query.getSeriesType(), operation );
            }
            else {
                throw new IllegalArgumentException(
                    "start end timestamp delta is invalid" );
            }
        }

        throw new IllegalArgumentException( "Date Range is invalid" );    	
    }
    
    private List<Series> hourRollup( final List<VolumeDatapoint> datapoints, 
    								 final Long epoch,
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
    		
    		series.add( generate( datapoints, epoch, metric, 2L, 30, operation ) );
    	}
    	
    	return series;
    }
    
    private List<Series> dayRollup(	final List<VolumeDatapoint> datapoints, 
			 						final Long epoch,
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
			series.add( generate( datapoints, epoch, metric, 60L, 30, operation ) );
		}
	
		return series;
	}    
    
    private List<Series> weekRollup(final List<VolumeDatapoint> datapoints, 
									final Long epoch,
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
			series.add( generate( datapoints, epoch, metric, 360L, 28, operation ) );
		}
		
		return series;
	}      
    
    private List<Series> thirtyDaysRollup(	final List<VolumeDatapoint> datapoints, 
											final Long epoch,
											final List<Metrics> metrics,
											final StatOperation operation ) {

		final List<Series> series = new ArrayList<Series>();
		
        /*
         * for each volume
         *  sum all datapoints for each day to produce one datapoint,
         *  per volume
         */
		for ( Metrics metric : metrics ) {
			series.add( generate( datapoints, epoch, metric, 1440L, 30, operation ) );
		}
		
		return series;
	}  
    
    private List<Series> longTermRollup( final List<VolumeDatapoint> datapoints,
    		final long epoch,
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
			series.add( generate( datapoints, epoch, metric, 2880L, 999, operation ) );
		}
		
		return series;
    }

    /**
     * This method will take the list and parse it down to only
     * values that match the metric argument.  Then it will
     * sum these values into buckets defined by the distribution value (in seconds).
     * 
     * If requested (operation = RATE), it will transform those buckets into rates based on the distribution.
     * 
     * It will then map these into a series object ready for display in a chart
     * or other statistical display mechanisms
     * 
     * @param volumeDatapoints
     * @param timestamp
     * @param metrics
     * @param distribution
     * @param maxResults
     * @param operation
     * @return
     */
    private Series generate(
        final List<VolumeDatapoint> volumeDatapoints,
        final Long timestamp,
        final Metrics metrics,
        final Long distribution,
        final int maxResults,
        final StatOperation operation ) {
    	
        Map<Long, Set<VolumeDatapoint>> groupByTimestamp =
            vdpHelper.groupByTimestamp( volumeDatapoints );

        final List<Datapoint> datapoints = new ArrayList<>( );
        groupByTimestamp.forEach( ( bytesTimestamp, bytesValues ) -> {
            
        	// this is always a sum because it represents collapsing
        	// a set of points that are cumulative numbers by nature
        	// i.e. if you are wanting the average number of PUTS over a time frame
        	// you still need to sum them here, and divide that number by the 
        	// distribution in order to get a real average.  Otherwise you'll
        	// be averaging an average which will be significantly lower than
        	// the value you desire.
        	Double d = bytesValues.stream()
                                        .filter( ( value ) -> {
                                        	return value.getKey().equalsIgnoreCase( metrics.key() );
                                        })
                                        .peek( ( l ) -> logger.trace( l.toString() ) )
                                        .mapToDouble( VolumeDatapoint::getValue )
                                        .sum();
             
            logger.trace( "DOUBLE::{} LONG::{} TIMESTAMP::{}",
                          d, d.longValue(), bytesTimestamp );

            datapoints.add( new DatapointBuilder().withY( d )
                                                  .withX( (double)bytesTimestamp )
                                                  .build() );
        } );

        logger.trace( "START::{} INTERVAL::{} MAX::{} SIZE::{}",
                      timestamp, distribution, maxResults, datapoints.size() );

        final List<Datapoint> results = new ArrayList<>( );
        
        // we need to create a bucket for each "distribution" size counting back from
        // the start time maxResults number of times.
        Map<Double, List<Datapoint>> bucketMap = new HashMap<Double, List<Datapoint>>();
        
        for ( Datapoint dp : datapoints ){
        	
        	// normalize the key so it's the value of one of our buckets
        	// which are separated by "distribution" and start at "timestamp"
        	Double diff = dp.getX() - timestamp;
        	
        	Double bucket = Math.floor( diff.doubleValue() / new Double( TimeUnit.MINUTES.toSeconds( distribution ) ) );
        	bucket = timestamp + (bucket * TimeUnit.MINUTES.toSeconds( distribution ) );
        	
        	List<Datapoint> bucketList = bucketMap.get( bucket );
        	
        	if ( bucketList == null ){
        		bucketList = new ArrayList<Datapoint>();
        	}

    		bucketList.add( dp );
    		bucketMap.put( bucket,  bucketList );
        }
        
        for ( Double key : bucketMap.keySet() ){
        	
        	Double rolledupValue = 0.0;
        	DoubleStream ds = bucketMap.get( key ).stream().mapToDouble( Datapoint::getY );
        	
        	switch( operation ) {
        		case RATE:
        			rolledupValue = ds.average().getAsDouble() / TimeUnit.MINUTES.toSeconds( distribution );
        			break;
        		default:
        			// the data has already been acted on, so everything in this bucket
        			// is already the sum of previous roll ups.  Therefore, the most recent
        			// value is what we want to use.  This is an assumption that things aren't
        			// getting smaller within the window.
        			rolledupValue = ds.max().getAsDouble();
        			break;
        	}
        	
        	results.add( new DatapointBuilder().withX( key ).withY( rolledupValue ).build() );
        }
        
        results.sort( ( dp1, dp2 ) -> dp1.getX().compareTo( dp2.getX() ) );
        
        if ( results.isEmpty() ){
        	
        	// at start time
        	results.add( new DatapointBuilder()
        		.withX( (double)timestamp )
        		.withY( 0.0 ).build() );
        	
        	// at end time
        	results.add( new DatapointBuilder()
        		.withX( (double)timestamp + (maxResults * TimeUnit.MINUTES.toSeconds( distribution ) ) )
        		.withY( 0.0 ).build() );
        }
        
        // if our earliest timestamp is after the requested start time we will add a zero "distribution" 
        // before the earliest, and a zero at the requested start time.
        else if ( results.get( 0 ).getX() > timestamp ){
        	
        	// a point just earlier than the first real point. ... let's do one second
        	results.add( 0, new DatapointBuilder()
        		.withX( results.get( 0 ).getX() - 1 )
        		.withY( 0.0 ).build() );
        	
        	// at start time
        	results.add( 0, new DatapointBuilder()
        		.withX( (double)timestamp )
        		.withY( 0.0 ).build() );
        }
       
        return new SeriesBuilder().withType( metrics )
                                  .withDatapoints( results )
                                  .build();
    }
}
