/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonMetricsRepository;
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

//    final Map<String,List<VolumeFirebreak>> allCapacity =
//      findFirebreak(
//        new VolumeCriteriaQueryBuilder( repo.entity() )
//          .withSeries( Metrics.STC_SIGMA )
//          .withSeries( Metrics.LTC_SIGMA ).resultsList() );
//
//    final Map<String,List<VolumeFirebreak>> allPerformance =
//      findFirebreak(
//        new VolumeCriteriaQueryBuilder( repo.entity() )
//          .withSeries( Metrics.STP_SIGMA )
//          .withSeries( Metrics.LTP_SIGMA ).resultsList() );

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
   * @return Returns {@link Map} of {@link List} of {@link VolumeFirebreak}
   */
  public static Map<String, List<Datapoint>> findFirebreak(
    final List<VolumeDatapoint> datapoints ) {
    final Map<String, List<Datapoint>> results = new HashMap<>();

    datapoints.stream()
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
                final String volumeName = o.getKey()
                                           .getVolumeName();
                final Date x = new Date( o.getKey()
                                          .getTimestamp() * 1000 );
                final Datapoint firebreak = new Datapoint();

                if( Calculation.isFirebreak( o.getKey()
                                              .getValue(),
                                             o.getValue()
                                              .getValue() ) ) {
                  if( !results.containsKey( volumeName ) ) {
                    results.put( volumeName, new ArrayList<>() );
                  }

                  if( !results.get( volumeName )
                              .contains( firebreak ) ) {
                    results.get( volumeName )
                           .add( firebreak );
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
