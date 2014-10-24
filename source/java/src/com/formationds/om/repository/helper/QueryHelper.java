/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.abs.Calculated;
import com.formationds.commons.model.abs.Metadata;
import com.formationds.commons.model.builder.DatapointBuilder;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.capacity.CapacityConsumed;
import com.formationds.commons.model.capacity.CapacityDeDupRatio;
import com.formationds.commons.model.capacity.CapacityFull;
import com.formationds.commons.model.capacity.CapacityToFull;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonMetricsRepository;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.VolumeCriteriaQueryBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;

/**
 * @author ptinius
 */
public class QueryHelper {
  private static final transient Logger logger =
    LoggerFactory.getLogger( QueryHelper.class );

  private final MetricsRepository repo;

  /**
   * default constructor
   */
  public QueryHelper() {
    this.repo =
      SingletonMetricsRepository.instance()
                                .getMetricsRepository();
  }

  /**
   * @param query the {@link QueryCriteria} representing the query
   *
   * @return Returns the {@link Statistics} representing the result of {@code query}
   */
  public Statistics execute( final QueryCriteria query ) {
    final Statistics stats = new Statistics();
    if( query != null ) {
      final List<Series> series = new ArrayList<>( );

      final List<VolumeDatapoint> queryResults =
        new VolumeCriteriaQueryBuilder( repo.entity() ).searchFor( query )
                                                       .resultsList();
      Map<String, Datapoint> firebreakPoints =
        new FirebreakHelper().findFirebreak( queryResults );
      if( !firebreakPoints.isEmpty() ) {
        logger.trace( "Gathering firebreak details");
        final Set<String> keys = firebreakPoints.keySet();
        for( final String key : keys ) {
          logger.trace( "Gathering firebreak for '{}'", key );
          final Series s =
            new SeriesBuilder().withContext(
              // TODO fully populate the Volume object
              new VolumeBuilder().withName( key )
//                                 .withId(  )
                                 .build() )
                               .withDatapoint( firebreakPoints.get( key ) )
                               .build();
          logger.trace( "firebreak series: {}", s );
          series.add( s );
        }
      }

      // TODO add check for firebreak, if is a firebreak query then skip everything below!

      for( final Metrics m : query.getSeriesType() ) {
        logger.trace( "Gathering statistics for '{}'", m.key() );
        switch( m.name() ) {
          case "PUTS":    // number of puts
            series.addAll( nonFireBreak( queryResults, Metrics.PUTS ) );
            break;
          case "GETS":    // number of gets
            series.addAll( nonFireBreak( queryResults, Metrics.GETS ) );
            break;
          case "QFULL":   // queue full
            series.addAll( nonFireBreak( queryResults, Metrics.QFULL ) );
            break;
          case "PSSDA":   // percent of SSD accesses
            series.addAll( nonFireBreak( queryResults, Metrics.PSSDA ) );
            break;
/*
 * logical and physical bytes are used to calculate de-duplication ratio.
 * so for now lets not allow the to be queried individually.
 *         case "LBYTES":  // logical bytes
 *           break;
 *         case "PBYTES":  // physical bytes
 *           break;
 */
          case "BLOBS":   // number of blobs
            series.addAll( nonFireBreak( queryResults, Metrics.BLOBS ) );
            break;
          case "OBJECTS": // number of objects
            series.addAll( nonFireBreak( queryResults, Metrics.OBJECTS ) );
            break;
          case "ABS":     // average object size
            series.addAll( nonFireBreak( queryResults, Metrics.ABS ) );
            break;
          case "AOPB":    // average objects per blob
            series.addAll( nonFireBreak( queryResults, Metrics.AOPB ) );
            break;
          // all other metrics are considered firebreak, so ignore them here
        }
      }

      if( !series.isEmpty() ) {
        stats.setSeries( series );
      }
      stats.setCalculated( calculated( queryResults ) );
    }

    return stats;
  }

  /**
   * @param queryResults the {@link List} of {@link VolumeDatapoint}
   * @param metrics the {@link Metrics} representing the none firebreak metric
   *
   * @return Returns q {@link List} of {@link Series}
   */
  protected List<Series> nonFireBreak( final List<VolumeDatapoint> queryResults,
                                       final Metrics metrics ) {
    final List<Series> series = new ArrayList<>( );
    final Map<String,List<VolumeDatapoint>> mapped = organize( queryResults );

    mapped.forEach( ( key, volumeDatapoints ) -> {
      final Series s = new Series();
      volumeDatapoints.stream()
                      .filter( ( p ) -> metrics.key()
                                               .equalsIgnoreCase( p.getKey() ) )
                      .forEach( ( p ) -> {
                        final Datapoint dp =
                          new DatapointBuilder().withX(
                            DateTimeUtil.epochToMilliseconds( p.getTimestamp() ) )
                                                .withY( p.getValue().longValue() )
                                                .build();
                          s.setDatapoint( dp );
                          s.setType( metrics );
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
   * @param vdp the {@link List} of {@link VolumeDatapoint}
   * @param metrics the {@link Metrics}
   *
   * @return Returns {@link VolumeDatapoint} representing the last data point
   *         found for {@code metrics}
   */
  protected VolumeDatapoint lastDataPoint( final List<VolumeDatapoint> vdp,
                                           final Metrics metrics ) {
    final VolumeDatapoint[ ] last = { null };
    vdp.stream()
       .filter( v -> v.getKey().equalsIgnoreCase( metrics.key() ) )
       .forEach( v -> {
         if( last[ 0 ] == null ) {
           last[ 0 ] = v;
         } else if( last[ 0 ].getTimestamp() < v.getTimestamp() ) {
           last[ 0 ] = v;
         }
       } );

    return last[ 0 ];
  }

  /**
   * @return Returns a {@link List} of {@link Calculated}
   */
  protected List<Calculated> calculated(
    final List<VolumeDatapoint> queryResults ) {
    final List<Calculated> calculated = new ArrayList<>( );

    final Map<String,List<VolumeDatapoint>> mapped = organize( queryResults );

    final List<VolumeDatapoint> physical = new ArrayList<>( );
    final List<VolumeDatapoint> logical = new ArrayList<>( );
    mapped.forEach( ( key, volumeDatapoints ) -> {
      final VolumeDatapoint lbytes = lastDataPoint( volumeDatapoints,
                                                    Metrics.LBYTES );
      if( lbytes != null ) {
        logical.add( lbytes );
      }
      final VolumeDatapoint pbytes = lastDataPoint( volumeDatapoints,
                                                    Metrics.PBYTES );
      if( pbytes != null ) {
        physical.add( pbytes );
      }
    } );

    calculated.add( deDupRatio( logical, physical ) );
    calculated.add( bytesConsumed( logical ) );
    calculated.add( percentageFull() );
    calculated.add( toFull() );

    return calculated;
  }

  /**
   * @return Returns a {@link List} of {@link Metadata}
   */
  protected List<Metadata> metadata() {
    return new ArrayList<>( );
  }

  /**
   * @param logical the {@link List} of last {@link VolumeDatapoint} from each series
   * @param physical the {@link List} of last {@link VolumeDatapoint} from each series
   *
   * @return Returns {@link CapacityDeDupRatio}
   */
  protected CapacityDeDupRatio deDupRatio( final List<VolumeDatapoint> logical,
                                           final List<VolumeDatapoint> physical ) {
    final Double lbytes =
      logical.stream()
              .mapToDouble( VolumeDatapoint::getValue )
              .summaryStatistics()
              .getSum();

    final Double pbytes =
      physical.stream()
              .mapToDouble( VolumeDatapoint::getValue )
              .summaryStatistics()
              .getSum();

    return new CapacityDeDupRatio( Calculation.ratio( lbytes, pbytes ) );
  }

  /**
   * @param logical the {@link List} of last {@link VolumeDatapoint} from each series
   *
   * @return Returns {@link CapacityConsumed}
   */
  protected CapacityConsumed bytesConsumed( final List<VolumeDatapoint> logical ) {
    return new CapacityConsumed( logical.stream()
                                        .mapToDouble( VolumeDatapoint::getValue )
                                        .summaryStatistics()
                                        .getSum() );
  }

  /**
   * @return Returns {@link CapacityFull}
   */
  protected CapacityFull percentageFull() {
    // TODO finish implementation
    return new CapacityFull( 50 );
  }

  /**
   * @return Returns {@link CapacityFull}
   */
  protected CapacityToFull toFull() {
    // TODO finish implementation
    return new CapacityToFull( 24 );
  }

  /**
   * @param datapoints the {@link List} of {@link VolumeDatapoint}
   *
   * @return Return {@link Map} representing volumes as the keys and value
   *         representing a {@link List} of {@link VolumeDatapoint}
   */
  protected static Map<String,List<VolumeDatapoint>> organize(
    final List<VolumeDatapoint> datapoints ) {
    final Map<String,List<VolumeDatapoint>> mapped = new HashMap<>( );

    final Comparator<VolumeDatapoint> VolumeDatapointComparator =
      Comparator.comparing( VolumeDatapoint::getVolumeName )
                .thenComparing( VolumeDatapoint::getTimestamp );

    datapoints.stream()
              .sorted( VolumeDatapointComparator )
              .forEach( ( v ) -> {
                if( !mapped.containsKey( v.getVolumeName() ) ) {
                  mapped.put( v.getVolumeName(), new ArrayList<>( ) );
                }

                mapped.get( v.getVolumeName() ).add( v );
              } );

    return mapped;
  }
}
