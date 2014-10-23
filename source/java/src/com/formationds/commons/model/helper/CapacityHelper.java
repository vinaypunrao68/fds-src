/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.capacity.CapacityDeDupRatio;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.SingletonMetricsRepository;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.builder.QueryCriteriaBuilder;
import com.formationds.om.repository.result.VolumeDatapointList;

/**
 * @author ptinius
 */
public class CapacityHelper {
  /**
   * @param volumeName the {@link String} representing the volume name
   * @param volumeId   the {@link Long} representing the volume id
   *
   * @return Returns {@link com.formationds.commons.model.capacity.CapacityDeDupRatio}
   */
  public static CapacityDeDupRatio calculateDedupRatio(
    final String volumeName,
    final Long volumeId ) {

    final Volume volume = new VolumeBuilder().withId( String.valueOf( volumeId ) )
                                             .withName( volumeName )
                                             .build();
    final QueryCriteria criteria =
      new QueryCriteriaBuilder().withSeriesType( Metrics.LBYTES )
                                .withSeriesType( Metrics.PBYTES )
                                .withContexts( volume )
                                .withPoints( 2 )
                                .build();
    final VolumeDatapointList results =
      SingletonMetricsRepository.instance()
                                .getMetricsRepository()
                                .query( criteria );
    Double enumerator = null;
    Double denominator = null;
    for( final VolumeDatapoint vdp : results ) {
      if( vdp.getKey()
             .equalsIgnoreCase( Metrics.PBYTES.name() ) ) {
        enumerator = vdp.getValue();
      } else if( vdp.getKey()
                    .equalsIgnoreCase( Metrics.LBYTES.name() ) ) {
        denominator = vdp.getValue();
      }
    }

    if( enumerator != null && denominator != null ) {
      return new CapacityDeDupRatio( volumeName, Calculation.ratio( enumerator,
                                                        denominator ) );
    }

    throw new IllegalArgumentException();
  }
}
