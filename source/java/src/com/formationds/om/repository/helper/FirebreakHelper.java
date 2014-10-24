/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.DateTimeUtil;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.om.repository.result.VolumeFirebreak;
import javafx.util.Pair;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;

/**
 * @author ptinius
 */
public class FirebreakHelper {
  private static final transient Logger logger =
    LoggerFactory.getLogger( FirebreakHelper.class );

  private static List<Metrics> FIREBREAKS = new ArrayList<>( );
  static {
    FIREBREAKS.add( Metrics.STC_SIGMA );
    FIREBREAKS.add( Metrics.LTC_SIGMA );
    FIREBREAKS.add( Metrics.STP_SIGMA );
    FIREBREAKS.add( Metrics.LTP_SIGMA );
    FIREBREAKS.add( Metrics.STC_WMA );
    FIREBREAKS.add( Metrics.LTC_WMA );
  }

  /**
   * default constructor
   */
  public FirebreakHelper() {
  }

  /**
   * @param datapoints the {@link VolumeDatapoint} representing the datapoint
   *
   * @return Returns {@link java.util.Map} of {@link Datapoint}
   */
  public Map<String, Datapoint> findFirebreak(
    final List<VolumeDatapoint> datapoints ) {
    final Map<String, Datapoint> results = new HashMap<>();

    final Comparator<VolumeDatapoint> VolumeDatapointComparator =
      Comparator.comparing( VolumeDatapoint::getVolumeName )
                .thenComparing( VolumeDatapoint::getTimestamp );

    final VolumeFirebreak[] previous = { null };

    datapoints.stream()
              .sorted( VolumeDatapointComparator )
              .flatMap( ( dp1 ) ->
                          datapoints.stream()
                                    .filter( this::isFirebreakType )
                                    .filter( ( dp2 ) ->
                                               dp2.getVolumeName()
                                                  .equalsIgnoreCase( dp1.getVolumeName() ) )
                                    .filter( ( dp2 ) ->
                                               dp1.getTimestamp()
                                                  .equals(
                                                    dp2.getTimestamp() ) )
                                    .filter( ( dp2 ) -> !dp1.getId()
                                                            .equals( dp2.getId() ) )
                                    .filter( this::isFirebreakType )
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
                                         new Date(
                                           DateTimeUtil.epochToMilliseconds(
                                             v1.getTimestamp()
                                                                           )
                                         )
                    );
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

  private boolean isFirebreakType( final VolumeDatapoint vdp ) {
    try {
      return FIREBREAKS.contains( Metrics.byMetadataKey( vdp.getKey() ) );
    } catch( UnsupportedMetricException e ) {
      logger.warn( e.getMessage() );
      logger.trace( ExceptionHelper.toString( e ) );
      return false;
    }
  }

  private static Pair<VolumeDatapoint, VolumeDatapoint> build(
    final VolumeDatapoint dp1,
    final VolumeDatapoint dp2 ) {

    return new Pair<>( dp1, dp2 );
  }
}
