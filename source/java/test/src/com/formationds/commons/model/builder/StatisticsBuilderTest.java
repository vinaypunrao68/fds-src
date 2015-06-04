/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Size;
import com.formationds.client.v08.model.SizeUnit;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.VolumeState;
import com.formationds.client.v08.model.VolumeStatus;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class StatisticsBuilderTest {
  private static final String EXPECTED_NAME =
    "TestVolume %s";
  private static final int EXPECTED_LIMIT = 100;
  private static final int EXPECTED_SLA = 0;
  private static final int EXPECTED_PRIORITY = 10;

  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GIBIBYTE;
  private static final long     EXPECTED_SIZE  = 100;
  private static final long   EXPECTED_ID    = 91237403;

    private static final Size EXP_SIZE = Size.of( EXPECTED_SIZE, EXPECTED_UNITS );
    private static final VolumeStatus status = new VolumeStatus( VolumeState.Active, EXP_SIZE );
    private static final Volume volume =
        new Volume.Builder( String.format( EXPECTED_NAME, "1" ) )
            .id( EXPECTED_ID )
            .status( status )
            .qosPolicy( new QosPolicy( EXPECTED_PRIORITY, EXPECTED_SLA, EXPECTED_LIMIT ) )
            .create();

    final static List<Series> series = new ArrayList<>();

    static {
        series.add( new SeriesBuilder().withContext( volume )
                                       .withDatapoint( new DatapointBuilder().withX( 55.0 )
                                                                             .withY( 123456789.0 )
                                                                             .build() )
                                       .withDatapoint( new DatapointBuilder().withX( 15.0 )
                                                                             .withY( 23456789.0 )
                                                                             .build() )
                                       .build() );
    }

    @Test
    public void testWithSeries()
        throws Exception {
        final Statistics stat =
            new StatisticsBuilder().withSeries( series )
                                   .build();
        System.out.println( ObjectModelHelper.toJSON( stat ) );
    }

    @Test
    public void testAddSeries()
        throws Exception {
        final Statistics stat =
            new StatisticsBuilder().addSeries( new SeriesBuilder().withContext( volume )
                                                                  .withDatapoint( new DatapointBuilder().withX( 55.0 )
                                                                                                        .withY( 123456789.0 )
                                                                                                        .build() )
                                                            .build() )
                             .addSeries( new SeriesBuilder().withContext( volume )
                                                            .withDatapoint( new DatapointBuilder().withX( 15.0 )
                                                                                                  .withY( 23456789.0 )
                                                                                                  .build() )
                                                            .build() )
                             .build();
    System.out.println( ObjectModelHelper.toJSON( stat ) );
  }

  @Test
  public void testWithCalculated()
    throws Exception {

  }

  @Test
  public void testAddCalculated()
    throws Exception {

  }

  @Test
  public void testWithMetadata()
    throws Exception {

  }

  @Test
  public void testAddSeries1()
    throws Exception {

  }

  @Test
  public void testBuild()
    throws Exception {

  }
}