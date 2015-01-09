/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Events;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.abs.Calculated;
import com.formationds.commons.model.abs.Context;
import com.formationds.commons.model.abs.Metadata;
import com.formationds.commons.model.builder.DatapointBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.calculated.capacity.AverageIOPs;
import com.formationds.commons.model.calculated.capacity.CapacityConsumed;
import com.formationds.commons.model.calculated.capacity.CapacityDeDupRatio;
import com.formationds.commons.model.calculated.capacity.CapacityFull;
import com.formationds.commons.model.calculated.capacity.CapacityToFull;
import com.formationds.commons.model.calculated.firebreak.FirebreaksLast24Hours;
import com.formationds.commons.model.calculated.performance.IOPsConsumed;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.StatOperation;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.MetricCriteriaQueryBuilder;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.util.SizeUnit;
import com.formationds.xdi.ConfigurationApi;

import org.apache.commons.math3.stat.regression.SimpleRegression;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;
import java.util.stream.DoubleStream;

import javax.persistence.EntityManager;

/**
 * @author ptinius
 */
@SuppressWarnings( "UnusedDeclaration" )
public class QueryHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( QueryHelper.class );

    private final MetricsRepository repo;

    /**
     * default constructor
     */
    public QueryHelper() {
        this.repo =
            SingletonRepositoryManager.instance()
                                      .getMetricsRepository();
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Return {@link Map} representing volumes as the keys and value
     * representing a {@link List} of {@link VolumeDatapoint}
     */
    protected static Map<String, List<VolumeDatapoint>> byVolumeNameTimestamp(
        final List<VolumeDatapoint> datapoints ) {
        final Map<String, List<VolumeDatapoint>> mapped = new HashMap<>();

        final Comparator<VolumeDatapoint> VolumeDatapointComparator =
            Comparator.comparing( VolumeDatapoint::getVolumeName )
                      .thenComparing( VolumeDatapoint::getTimestamp );

        datapoints.stream()
                  .sorted( VolumeDatapointComparator )
                  .forEach( ( v ) -> {
                      if( !mapped.containsKey( v.getVolumeName() ) ) {
                          mapped.put( v.getVolumeName(), new ArrayList<>() );
                      }

                      mapped.get( v.getVolumeName() )
                            .add( v );
                  } );

        return mapped;
    }

    /**
     * @param query the {@link com.formationds.om.repository.query.MetricQueryCriteria} representing the query
     *
     * @return Returns the {@link Statistics} representing the result of {@code
     * query}
     */
    @SuppressWarnings( "unchecked" )
    public Statistics execute( final MetricQueryCriteria query, final Authorizer authorizer, final AuthenticationToken token )
        throws TException {
        final Statistics stats = new Statistics();
        if( query != null ) {
            final List<Series> series = new ArrayList<>();
            final List<Calculated> calculatedList = new ArrayList<>();

            EntityManager em = repo.newEntityManager();
            try {
            	
            	query.setContexts( validateContextList( query, authorizer, token ) );
            	
	            final List<VolumeDatapoint> queryResults =
	                new MetricCriteriaQueryBuilder( em ).searchFor( query )
	                                                               .resultsList();
	            final Map<String, List<VolumeDatapoint>> originated =
	                byVolumeNameTimestamp( queryResults );
	
	            if( isFirebreakQuery( query.getSeriesType() ) ) {
	
	                series.addAll( new FirebreakHelper().processFirebreak( queryResults ) );
	                final FirebreaksLast24Hours firebreak = new FirebreaksLast24Hours();
	                firebreak.setCount( last24Hours( series ) );
	                calculatedList.add( firebreak );
	
	            } else if( isPerformanceQuery( query.getSeriesType() ) ) {
	
	                series.addAll(
	                    new SeriesHelper().getRollupSeries( queryResults,
	                                                        query,
	                                                        StatOperation.RATE ) );
	                final IOPsConsumed ioPsConsumed = new IOPsConsumed();
	                ioPsConsumed.setDailyAverage( 0.0 );
	                calculatedList.add( ioPsConsumed );
	
	            } else if( isCapacityQuery( query.getSeriesType() ) ) {
	            	
	                series.addAll(
	                    new SeriesHelper().getRollupSeries( queryResults,
	                                                        query,
	                                                        StatOperation.SUM) );
	
	                calculatedList.add( deDupRatio() );
	
	                final CapacityConsumed consumed = bytesConsumed();
	                calculatedList.add( consumed );
	
	                if ( authorizer.userFor( token ).isFdsAdmin ){
		                // TODO finish implementing -- once the platform has total system capacity
		                final Double systemCapacity = Long.valueOf( SizeUnit.TB.totalBytes( 1 ) )
		                                                  .doubleValue();
//		                calculatedList.add( percentageFull( consumed, systemCapacity ) );
		                
		                // TODO finish implementing  -- once Nate provides a library
		            	Series physicalBytes = series.stream()
		            		.filter( ( s ) -> { 
		            			return s.getType().equals( Metrics.PBYTES.name() );
		            		})
			            	.findFirst().orElse( null );
		            	
		            	if ( physicalBytes != null ){
		            		calculatedList.add( toFull( physicalBytes, systemCapacity ) );
		            	}
		            	else {
		            		logger.info( "There were no physical bytes reported for the system.  Cannot calculate time to full.");
		            	}
	                }
	
	            } else if ( isPerformanceBreakdownQuery( query.getSeriesType() ) ) {
	            	
	            	normalizeDatapoints( queryResults );
	            	
	            	series.addAll(
	            		new SeriesHelper().getRollupSeries( queryResults, 
	            										 	query,
	            										 	StatOperation.RATE) );
	            	
	            	// GETS has the total # of gets and SSD is a subset of those.
	            	// This query wants GETS for HDD access and SSD access so we mutate the
	            	// GETS series with a quick subtraction of the corresponding SSD field
	            	Series gets = series.stream().filter( s -> s.getType().equals( Metrics.GETS.name() ) )
	            		.findFirst().get();
	            	
	            	Series ssdGets = series.stream().filter( s -> s.getType().equals( Metrics.SSD_GETS.name() ) )
	            		.findFirst().get();
	            	
	            	gets.getDatapoints().forEach( 
	            		gPoint -> {
	            			ssdGets.getDatapoints().stream().filter( sPoint -> sPoint.getX().equals( gPoint.getX() ) )
	            				.forEach(
		            				sPoint -> {
		            					gPoint.setY( gPoint.getY() - sPoint.getY() );
		            				}
		            			);
	            		});
	            	
	            	calculatedList.add( getAverageIOPs( series ) );
	            	
	            } else {
	            	
	                // individual stats
	                query.getSeriesType()
	                     .stream()
	                     .forEach( ( m ) ->
	                         series.addAll( otherQueries( originated,
	                                                      m ) ) );
	            }
	
	            if( !series.isEmpty() ) {
	                series.forEach( ( s ) ->
	                    new DatapointHelper().sortByX( s.getDatapoints() ) );
	                stats.setSeries( series );
	            }
	
	            if( !calculatedList.isEmpty() ) {
	                stats.setCalculated( calculatedList );
	            }

            } finally {
            	em.close();
            }
        }

        return stats;
    }

    /**
     *
     * @param query the {@link com.formationds.om.repository.query.MetricQueryCriteria} representing the query
     * @return the events matching the query criteria
     * @throws TException
     */
    public Events executeEventQuery( final QueryCriteria query )
            throws TException {
        EventRepository er = SingletonRepositoryManager.instance().getEventRepository();

        return er.query(query);
    }

    /**
     * @param metrics the [@link List} of {@link Metrics}
     *
     * @return Returns {@code true} if all {@link Metrics} within the
     *         {@link List} are of performance type. Otherwise {@code false}
     */
    protected boolean isPerformanceQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !Metrics.PERFORMANCE.contains( m ) ) {
                return false;
            }
        }

        return true;
    }

    /**
     * @param metrics the [@link List} of {@link Metrics}
     *
     * @return Returns {@code true} if all {@link Metrics} within the
     *         {@link List} are of capacity type. Otherwise {@code false}
     */
    protected boolean isCapacityQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !Metrics.CAPACITY.contains( m ) ) {
                return false;
            }
        }

        return true;
    }

    /**
     * @param metrics the [@link List} of {@link Metrics}
     *
     * @return Returns {@code true} if all {@link Metrics} within the
     *         {@link List} are of firebreak type. Otherwise {@code false}
     */
    protected boolean isFirebreakQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !Metrics.FIREBREAK.contains( m ) ) {
                return false;
            }
        }

        return true;
    }
    
    /**
     * 
     * @param determine if the {@link List} of {@link Metrics} matches the performance breakdown definition
     * 
     * @return returns true if all {@link Metrics} are included in both sets
     */
    protected boolean isPerformanceBreakdownQuery( final List<Metrics> metrics ) {
    	for ( final Metrics m : metrics ) {
    		if ( !Metrics.PERFORMANCE_BREAKDOWN.contains( m ) ){
    			return false;
    		}
    	}
    	
    	return true;
    }

    /**
     * @param organized the {@link Map} of volume containing a {@link List}
     *                  of {@link VolumeDatapoint}
     * @param metrics   the {@link Metrics} representing the none firebreak
     *                  metric
     *
     * @return Returns q {@link List} of {@link Series}
     */
    protected List<Series> otherQueries(
        final Map<String, List<VolumeDatapoint>> organized,
        final Metrics metrics ) {
        final List<Series> series = new ArrayList<>();

        organized.forEach( ( key, volumeDatapoints ) -> {
            final Series s = new Series();
            s.setType( metrics.name() );
            volumeDatapoints.stream()
                            .distinct()
                            .filter( ( p ) -> metrics.key()
                                                     .equalsIgnoreCase( p.getKey() ) )
                            .forEach( ( p ) -> {
                                final Datapoint dp =
                                    new DatapointBuilder().withX( (double)p.getTimestamp() )
                                                          .withY( p.getValue() )
                                                          .build();
                                s.setDatapoint( dp );
                                s.setContext(
                                    new VolumeBuilder().withName( p.getVolumeName() )
                                                       .withId( String.valueOf( p.getVolumeId() ) )
                                                       .build() );
                            } );
            series.add( s );
        } );

        return series;
    }

    /**
     * This method will run through all the results and normailze the
     * values so that the are reduced to values per second.
     * @param points
     */
    protected void normalizeDatapoints( final List<VolumeDatapoint> points ){
    	
    	points.stream().forEach( point -> {
    		
    		/**
    		 * In the future we'll have a calculation interval per datapoint
    		 * that is tied in to the data rollup strategy
    		 * and we can use that to normalize the data into per seconds.
    		 * 
    		 * For now we always do 2 minute intervals
    		 */
//    		long intervalInSeconds = point.getCalculationInterval();
    		final Double intervalInSeconds = new Double(1);
    		
    		point.setValue( point.getValue() / intervalInSeconds );
    	});
    }
    
    /**
     * This will look at the {@link Context} of the query and make sure the user has access
     * to them before allowing them to query for its information.  If there are no contexts,
     * it will fill them in with the {@link Volume}s the user can access.  
     * 
     * ** THIS ASSUMES CONTEXT IS A VOLUME **
     * If this assumption becomes false at some point then this method will need to change
     * in order to take that into consideration
     * 
     * @param query
     * @param token
     */
    protected List<Context> validateContextList( final MetricQueryCriteria query, final Authorizer authorizer, final AuthenticationToken token ){
    	
    	List<Context> contexts = query.getContexts();
    	
    	ConfigurationApi api = SingletonConfigAPI.instance().api();
    	
    	// fill it with all the volumes they have access to
    	if ( contexts.isEmpty() ){

    		try {
    		
	    		contexts = api.listVolumes("")
	    			.stream()
	    			.filter( vd -> authorizer.hasAccess( token, vd.getName() ) )
	    			.map( vd -> {
	    				
	    				String volumeId = "";
	    				
	    				try{
	    					volumeId = String.valueOf( api.getVolumeId( vd.getName() ) );
	    				}
	    				catch( TException e ){
	    					
	    				}
	    				
	    				Volume volume = new VolumeBuilder().withId( volumeId ).withName( vd.getName() )
	    					.build();
	    				
	    				return volume;
	    			})
	    			.collect( Collectors.toList() );
    		
    		} catch ( Exception e ){
    			logger.error( "Could not gather the volumes this user has access to.", e) ;
    		}
    	}
    	// validate the request matches the authorization
    	else {
    		
    		contexts = contexts.stream().filter( c -> {
    			boolean hasAccess = authorizer.hasAccess( token, ((Volume)c).getName() );
    			
    			if ( hasAccess == false ){
    				// TODO: Add an audit event here because someone may be trying an attack
    				logger.warn( "User does not have access to query for volume: " + ((Volume)c).getName() +
    					".  It will be removed from the query context." );
    			}
    			
    			return hasAccess;
    		})
    		.collect( Collectors.toList()); 
    		
    	}
    	
    	return contexts;
    }
    
    /**
     * @return Returns a {@link List} of {@link Metadata}
     */
    protected List<Metadata> metadata() {
        return new ArrayList<>();
    }
    
    /**
     * 
     * @param series
     * @return Returns the average IOPs for the collection of series passed in
     */
    protected AverageIOPs getAverageIOPs( List<Series> series ){
    	
    	Double rawAvg = series.stream().flatMapToDouble( s -> {
    		return DoubleStream.of( s.getDatapoints().stream()
    				.flatMapToDouble( dp -> DoubleStream.of( dp.getY() ) ).sum() / s.getDatapoints().size() );
    	}).sum();
 
    	
    	final AverageIOPs avgIops = new AverageIOPs();
    	avgIops.setAverage( rawAvg );
    	
    	return avgIops;
    }

    /**
     * @return Returns {@link CapacityDeDupRatio}
     */
    protected CapacityDeDupRatio deDupRatio() {
        final Double lbytes =
            SingletonRepositoryManager.instance()
                                      .getMetricsRepository()
                                      .sumLogicalBytes();

        final Double pbytes =
            SingletonRepositoryManager.instance()
                                      .getMetricsRepository()
                                      .sumPhysicalBytes();
        final CapacityDeDupRatio dedup = new CapacityDeDupRatio();
        dedup.setRatio( Calculation.ratio( lbytes, pbytes ) );
        return dedup;
    }

    /**
     * @return Returns {@link CapacityConsumed}
     */
    protected CapacityConsumed bytesConsumed() {
        final CapacityConsumed consumed = new CapacityConsumed();
        consumed.setTotal( SingletonRepositoryManager.instance()
                                                     .getMetricsRepository()
                                                     .sumPhysicalBytes() );
        return consumed;
    }

    /**
     * @param consumed       the {@link CapacityConsumed} representing bytes
     *                       used
     * @param systemCapacity the {@link Double} representing system capacity in
     *                       bytes
     *
     * @return Returns {@link CapacityFull}
     */
    protected CapacityFull percentageFull( final CapacityConsumed consumed,
                                           final Double systemCapacity ) {
        final CapacityFull full = new CapacityFull();
        full.setPercentage( ( int ) Calculation.percentage( consumed.getTotal(),
                                                            systemCapacity ) );
        return full;
    }

    /**
     * @return Returns {@link CapacityFull}
     */
    protected CapacityToFull toFull( final Series pSeries,  final Double systemCapacity ) {
        /*
         * TODO finish implementation
         * Add a non-linear regression for potentially better matching
         * 
         */
    	final SimpleRegression linearRegression = new SimpleRegression();
    	
    	pSeries.getDatapoints().stream().forEach( ( point ) -> {
    		linearRegression.addData( point.getX(), point.getY() );
    	});
    	
    	Double secondsToFull = systemCapacity / linearRegression.getSlope();
    	long days = TimeUnit.SECONDS.toDays( secondsToFull.longValue() );
    	
        final CapacityToFull to = new CapacityToFull();
        to.setToFull( days );
        return to;
    }

    /**
     * @return Returns {@link Integer} representing the number of firebreaks
     *         that have occurred in the last 24 hours.
     */
    protected Integer last24Hours( final List<Series> series ) {
        final AtomicInteger count = new AtomicInteger( 0 );
        final long twentyFourHoursAgo =
            DateTimeUtil.toUnixEpoch( LocalDateTime.now()
                                                   .minusHours( 24 ) );

        series.stream().forEach( ( s ) -> s.getDatapoints()
         .stream()
         .forEach( ( dp ) -> {
             if( dp.getY() >= twentyFourHoursAgo ) {
                 count.getAndIncrement();
             }
         } ) );

        return count.intValue();
    }
}
