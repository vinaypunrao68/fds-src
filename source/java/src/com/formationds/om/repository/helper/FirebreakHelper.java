/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.util.SizeUnit;
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

    /**
     * default constructor
     */
    public FirebreakHelper() {
    }

    private static Pair<VolumeDatapoint, VolumeDatapoint> build(
        final VolumeDatapoint dp1,
        final VolumeDatapoint dp2 ) {

        return new Pair<>( dp1, dp2 );
    }

    /**
     * @param datapoints the {@link VolumeDatapoint} representing the
     *                   datapoint
     *
     * @return Returns {@link java.util.Map} of {@link Datapoint}
     */
    public Map<String, Datapoint> findFirebreak(
        final List<VolumeDatapoint> datapoints ) {
        final Map<String, Datapoint> results = new HashMap<>();

        final Comparator<VolumeDatapoint> VolumeDatapointComparator =
            Comparator.comparing( VolumeDatapoint::getVolumeName )
                      .thenComparing( VolumeDatapoint::getTimestamp );

        final VolumeDatapoint[] previous = { null };

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

                      if( Calculation.isFirebreak( v1.getValue(), v2.getValue() ) ) {
                          if( previous[ 0 ] == null ) {
                              previous[ 0 ] = v2;
                          } else {
                              if( previous[ 0 ].getVolumeName()
                                               .equalsIgnoreCase( v2.getVolumeName() ) &&
                                  ( previous[ 0 ].getTimestamp() < v2.getTimestamp() ) ) {

                                  if( !results.containsKey( v2.getVolumeName() ) ) {
                                      final Datapoint datapoint = new Datapoint();
                                      // TODO get the size, i.e. capacity of the volume and set X
                                      // x: volume capacity
                                      datapoint.setX( SizeUnit.GB.totalBytes( 10 ) );
                                      // y: last occurred
                                      datapoint.setY( v2.getTimestamp() );

                                      results.put( v2.getVolumeName(), datapoint );
                                      previous[ 0 ] = v2;
                                  }
                              }
                          }
                      }
                  } );

        return results;
    }

    private boolean isFirebreakType( final VolumeDatapoint vdp ) {
        try {
            return QueryHelper.FIREBREAKS.contains( Metrics.byMetadataKey( vdp.getKey() ) );
        } catch( UnsupportedMetricException e ) {
            logger.warn( e.getMessage() );
            logger.trace( ExceptionHelper.toString( e ) );
            return false;
        }
    }
}
