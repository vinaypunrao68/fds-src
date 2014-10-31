/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.*;
import com.formationds.commons.model.abs.Calculated;
import com.formationds.commons.model.abs.Metadata;
import com.formationds.commons.model.builder.DatapointBuilder;
import com.formationds.commons.model.builder.DateRangeBuilder;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.calculated.capacity.CapacityConsumed;
import com.formationds.commons.model.calculated.capacity.CapacityDeDupRatio;
import com.formationds.commons.model.calculated.capacity.CapacityFull;
import com.formationds.commons.model.calculated.capacity.CapacityToFull;
import com.formationds.commons.model.calculated.firebreak.FirebreaksLast24Hours;
import com.formationds.commons.model.calculated.performance.IOPsCapacity;
import com.formationds.commons.model.calculated.performance.IOPsConsumed;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.query.MetricQueryCriteria;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.VolumeCriteriaQueryBuilder;
import com.formationds.util.SizeUnit;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.Timestamp;
import java.time.LocalDateTime;
import java.util.*;

/**
 * @author ptinius
 */
public class QueryHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( QueryHelper.class );

    private final MetricsRepository repo;

    public static List<Metrics> FIREBREAKS = new ArrayList<>();
    static {
        FIREBREAKS.add( Metrics.STC_SIGMA );
        FIREBREAKS.add( Metrics.LTC_SIGMA );
        FIREBREAKS.add( Metrics.STP_SIGMA );
        FIREBREAKS.add( Metrics.LTP_SIGMA );
    }

    public static List<Metrics> PERFORMANCE = new ArrayList<>();
    static {
        PERFORMANCE.add( Metrics.STP_WMA );
        // Currently the UI does not request
//        PERFORMANCE.add( Metrics.LTP_WMA );
    }

    public static final List<Metrics> CAPACITY = new ArrayList<>( );
    static {
        CAPACITY.add( Metrics.PBYTES );
        CAPACITY.add( Metrics.LBYTES );
        // Currently the UI does not request
//        CAPACITY.add( Metrics.STC_WMA );
//        CAPACITY.add( Metrics.LTC_WMA );
    }

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
    protected static Map<String, List<VolumeDatapoint>> organize(
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
    public Statistics execute( final MetricQueryCriteria query )
        throws TException {
        final Statistics stats = new Statistics();
        if( query != null ) {
            final List<Series> series = new ArrayList<>();

            final List<VolumeDatapoint> queryResults =
                new VolumeCriteriaQueryBuilder( repo.entity() ).searchFor( query )
                                                               .resultsList();
            if( isFirebreakQuery( query.getSeriesType() ) ) {
                Map<String, Datapoint> firebreakPoints =
                    new FirebreakHelper().findFirebreak( queryResults );

                if( !firebreakPoints.isEmpty() ) {
                    logger.trace( "Gathering firebreak details" );

                    final Set<String> keys = firebreakPoints.keySet();

                    for( final String key : keys ) {
                        logger.trace( "Gathering firebreak for '{}'", key );

                        final Series s =
                            new SeriesBuilder().withContext(
                                // TODO fully populate the Volume object
                                new VolumeBuilder().withName( key )
                                                   .build() )
                                               .withDatapoint( firebreakPoints.get( key ) )
                                               .build();
                        s.setType( "Firebreak" );
                        logger.trace( "firebreak series: {}", s );
                        series.add( s );
                    }
                } else {
                    logger.trace( "no firebreak data available" );
                }
            } else {
                for( final Metrics m : query.getSeriesType() ) {
                    logger.trace( "Gathering statistics for '{}'", m.key() );
                    switch( m ) {
                        case PUTS:    // number of puts
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case GETS:    // number of gets
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case QFULL:   // queue full
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case PSSDA:   // percent of SSD accesses
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
/*
 * logical and physical bytes are used to calculate de-duplication ratio.
 * so for now lets not allow the to be queried individually.
 *                      case LBYTES:  // logical bytes
 *                        break;
 *                      case PBYTES:  // physical bytes
 *                          break;
 */
                        case BLOBS:   // number of blobs
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case OBJECTS: // number of objects
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case ABS:     // average object size
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        case AOPB:    // average objects per blob
                            series.addAll( nonFireBreak( queryResults, m ) );
                            break;
                        // all other metrics are considered firebreak, so ignore them here
                    }
                }
            }

            if( !series.isEmpty() ) {
                stats.setSeries( series );
            }

            final List<Calculated> calculatedList = new ArrayList<>( );
            if( isFirebreakQuery( query.getSeriesType() ) ) {
                // TODO query to populate
                calculatedList.add( new FirebreaksLast24Hours( last24Hours() ) );
            } else if( isPerformanceQuery( query.getSeriesType() ) ) {
                // TODO query to populate
                calculatedList.add( new IOPsCapacity( 0.0 ) );
                calculatedList.add( new IOPsConsumed( 0.0 ) );
            } else if( isCapacityQuery( query.getSeriesType() ) ) {
                calculatedList.addAll( calculated() );
            }

            stats.setCalculated( calculatedList );
        }

        return stats;
    }

    public Events executeEventQuery( final QueryCriteria query )
            throws TException {

        return null;
    }

    protected boolean isPerformanceQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !PERFORMANCE.contains( m ) ) {
                return false;
            }
        }

        return true;
    }

    protected boolean isCapacityQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !CAPACITY.contains( m ) ) {
                return false;
            }
        }

        return true;
    }

    /**
     * @param metrics the [@link List} of {@link Metrics}
     *
     * @return Returns {@code true} if all {@link Metrics} within the {@link
     * List} are of firebreak type. Otherwise {@code false}
     */
    protected boolean isFirebreakQuery( final List<Metrics> metrics ) {
        for( final Metrics m : metrics ) {
            if( !FIREBREAKS.contains( m ) ) {
                return false;
            }
        }

        return true;
    }

    /**
     * @param datapoints the {@link List} of {!link VolumeDatapoint}
     *
     * @return Returns a {@link Map} of {@link VolumeDatapoint}
     */
    public Map<String, List<VolumeDatapoint>> byVolumeId(
        final List<VolumeDatapoint> datapoints ) {

        final Map<String, List<VolumeDatapoint>> byVolumeId = new HashMap<>( );

        final Comparator<VolumeDatapoint> VolumeDatapointComparator =
            Comparator.comparing( VolumeDatapoint::getVolumeId )
                      .thenComparing( VolumeDatapoint::getTimestamp )
                      .thenComparing( VolumeDatapoint::getKey );

        Collections.sort( datapoints, VolumeDatapointComparator );
        for( final VolumeDatapoint vdp : datapoints ) {
            final String key = vdp.getVolumeId();
            if( !byVolumeId.containsKey( key ) ) {
                byVolumeId.put( key, new ArrayList<>( ) );
            }

            byVolumeId.get( key ).add( vdp );
        }

        return byVolumeId;
    }

    /**
     * @param queryResults the {@link List} of {@link VolumeDatapoint}
     * @param metrics      the {@link Metrics} representing the none firebreak
     *                     metric
     *
     * @return Returns q {@link List} of {@link Series}
     */
    protected List<Series> nonFireBreak( final List<VolumeDatapoint> queryResults,
                                         final Metrics metrics ) {
        final List<Series> series = new ArrayList<>();
        final Map<String, List<VolumeDatapoint>> mapped = organize( queryResults );

        mapped.forEach( ( key, volumeDatapoints ) -> {
            final Series s = new Series();
            s.setType( metrics.name() );
            volumeDatapoints.stream()
                            .filter( ( p ) -> metrics.key()
                                                     .equalsIgnoreCase( p.getKey() ) )
                            .forEach( ( p ) -> {
                                final Datapoint dp =
                                    new DatapointBuilder().withX(
                                        DateTimeUtil.epochToMilliseconds( p.getTimestamp() ) )
                                                          .withY( p.getValue()
                                                                   .longValue() )
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
     * @return Returns a {@link List} of {@link Calculated}
     */
    protected List<Calculated> calculated() {
        final List<Calculated> calculated = new ArrayList<>();

        calculated.add( deDupRatio() );
        final CapacityConsumed consumed = bytesConsumed();
        calculated.add( consumed );
        // TODO finish implementing once the platform has total system capacity
        final Double systemCapacity = Long.valueOf( SizeUnit.TB.totalBytes( 1 ) )
                                          .doubleValue();
        calculated.add( percentageFull( consumed, systemCapacity ) );
        // TODO finish implementing once Nate provides a library
        calculated.add(toFull());

        return calculated;
    }

    /**
     * @return Returns a {@link List} of {@link Metadata}
     */
    protected List<Metadata> metadata() {
        return new ArrayList<>();
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

        return new CapacityDeDupRatio( Calculation.ratio( lbytes, pbytes ) );
    }

    /**
     * @return Returns {@link CapacityConsumed}
     */
    protected CapacityConsumed bytesConsumed() {
        return new CapacityConsumed( SingletonRepositoryManager.instance()
                                                               .getMetricsRepository()
                                                               .sumLogicalBytes() );
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
        return new CapacityFull(
            ( int ) Calculation.percentage( consumed.getTotal(),
                                            systemCapacity ) );
    }

    /**
     * @return Returns {@link CapacityFull}
     */
    protected CapacityToFull toFull() {
        // TODO finish implementation
        return new CapacityToFull( 24 );
    }

    /**
     * @return Returns {@link Integer} representing the number of firebreaks
     *         that have occurred in the last 24 hours.
     */
    protected Integer last24Hours() {
        final VolumeCriteriaQueryBuilder builder =
            new VolumeCriteriaQueryBuilder( repo.entity() );
        FIREBREAKS.stream().forEach( builder::withSeries );

        final LocalDateTime end = LocalDateTime.now();
        final LocalDateTime start = end.withHour( 23 );

        final DateRange last24hours =
            new DateRangeBuilder( ).withStart( new Timestamp( DateTimeUtil.toUnixEpoch( start ) ) )
                                   .withEnd( new Timestamp( DateTimeUtil.toUnixEpoch( end ) ) )
                                   .build();
        builder.withDateRange( last24hours );

        final Map<String,List<VolumeDatapoint>> organized =
            organize( builder.resultsList() );

        final int count[ ] = { 0 };
        organized.forEach( ( key, volumeDatapoints ) -> {
            volumeDatapoints.stream().forEach( ( vdp ) -> {
                if( Calculation.isFirebreak( 0.0, 0.0 ) ) {
                    count[ 0 ]++;
                }
            } );
        } );

        return count[ 0 ];
    }
}
