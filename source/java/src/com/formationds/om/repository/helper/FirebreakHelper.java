/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonMetricsRepository;
import com.formationds.om.repository.query.builder.VolumeCriteriaQueryBuilder;
import com.formationds.om.repository.result.VolumeFirebreak;
import javafx.util.Pair;

import java.util.*;

/**
 * @author ptinius
 */
public class FirebreakHelper {
  private final MetricsRepository repo;

  /**
   * default constructor
   */
  public FirebreakHelper() {
    this.repo =
      SingletonMetricsRepository.instance()
                                .getMetricsRepository();
  }

  /**
   * @return Returns {@link List} of {@link VolumeFirebreak}
   */
  public List<Datapoint> findFirebreak() {

    final Map<String, Datapoint> allCapacity =
      findFirebreak(
        new VolumeCriteriaQueryBuilder( repo.entity() )
          .withSeries( Metrics.STC_SIGMA )
          .withSeries( Metrics.LTC_SIGMA )
          .resultsList() );

    final Map<String, Datapoint> allPerformance =
      findFirebreak(
        new VolumeCriteriaQueryBuilder( repo.entity() )
          .withSeries( Metrics.STP_SIGMA )
          .withSeries( Metrics.LTP_SIGMA )
          .resultsList() );

    /*
     * should have one per volume, if any firebreaks have occurred.
     */

    /**
     * TODO finish implementation
     * need one fire break per volume, which ever is the most recent between
     * the capacity and performance
     */

    return null;
  }

  /**
   * @param datapoints the {@link VolumeDatapoint} representing the datapoint
   *
   * @return Returns {@link java.util.Map} of {@link Datapoint}
   */
  public static Map<String, Datapoint> findFirebreak(
    final List<VolumeDatapoint> datapoints ) {
    final Map<String, Datapoint> results = new HashMap<>();

    final Comparator<VolumeDatapoint> VolumeDatapointComprator =
      Comparator.comparing( VolumeDatapoint::getVolumeName )
                .thenComparing( VolumeDatapoint::getTimestamp );

    final VolumeFirebreak[] previous = { null };

    datapoints.stream()
              .sorted( VolumeDatapointComprator )
              .flatMap( ( dp1 ) ->
                          datapoints.stream()
                                    .filter( ( dp2 ) ->
                                               dp2.getVolumeName()
                                                  .equalsIgnoreCase( dp1.getVolumeName() ) )
                                    .filter( ( dp2 ) ->
                                               dp1.getTimestamp()
                                                  .equals(
                                                    dp2.getTimestamp() ) )
                                    .filter( ( dp2 ) -> !dp1.getId()
                                                            .equals( dp2.getId() ) )
                                    .map( dp2 -> build( dp1, dp2 ) ) )
              .forEach( o -> {
                final VolumeDatapoint v1 = o.getKey();
                final VolumeDatapoint v2 = o.getValue();

                if( !results.containsKey( v1.getVolumeName() ) ) {
                  final Datapoint empty = new Datapoint();
                  empty.setY( 0L );
                  empty.setX( 0L );
                  results.put( v1.getVolumeName(), empty );
                }

                if( Calculation.isFirebreak( v1.getValue(), v2.getValue() ) ) {
                  final VolumeFirebreak c =
                    new VolumeFirebreak( v1.getVolumeName(),
                                         v1.getId()
                                           .toString(),
                                         new Date( v1.getTimestamp() * 1000 ) );
                  if( previous[ 0 ] == null ) {
                    previous[ 0 ] = c;
                  }

                  if( previous[ 0 ].getVolumeName()
                                   .equalsIgnoreCase( c.getVolumeName() ) &&
                    previous[ 0 ].getLastOccurred()
                                 .before( c.getLastOccurred() ) ) {

                    results.get( previous[ 0 ].getVolumeName() )
                           .setY( v1.getValue()
                                    .longValue() );
                    results.get( previous[ 0 ].getVolumeName() )
                           .setX( previous[ 0 ].getLastOccurred()
                                               .getTime() );

                    previous[ 0 ] = c;
                  }
                }
              } );

    return results;
  }

  private static Pair<VolumeDatapoint, VolumeDatapoint> build(
    final VolumeDatapoint dp1,
    final VolumeDatapoint dp2 ) {

    return new Pair<>( dp1, dp2 );
  }
}
