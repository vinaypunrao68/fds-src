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
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Usage;
import com.formationds.commons.model.helper.ObjectModelHelper;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class SeriesBuilderTest {
  private static final String EXPECTED_NAME = "TestVolume %s";
  private static final int EXPECTED_LIMIT = 100;
  private static final int EXPECTED_SLA = 0;
  private static final int EXPECTED_PRIORITY = 10;

  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GIBIBYTE;
  private static final long     EXPECTED_SIZE  = 100;
  private static final Long     EXPECTED_ID    = 91237403L;

    private static final Size EXP_SIZE = Size.of( EXPECTED_SIZE, EXPECTED_UNITS );
    private static       Volume
                              VOLUME   = null;
    private static       Volume
                              VOLUME1  = null;

    @Before
    public void setUp() {
        final Usage usage =
            new UsageBuilder().withSize( String.valueOf( EXPECTED_SIZE ) )
                              .build();

        VolumeStatus status = new VolumeStatus( VolumeState.Active, EXP_SIZE );

        VOLUME =
            new Volume.Builder( String.format( EXPECTED_NAME, "1" ) )
                .id( EXPECTED_ID )
                .status( status )
                .qosPolicy( new QosPolicy( EXPECTED_PRIORITY, EXPECTED_SLA, EXPECTED_LIMIT ) )
                .create();

        VOLUME1 =
            new Volume.Builder( String.format( EXPECTED_NAME, "2" ) )
                .id( EXPECTED_ID )
                .status( status )
                .qosPolicy( new QosPolicy(EXPECTED_PRIORITY, EXPECTED_SLA, EXPECTED_LIMIT ) )
                .create();
    }

    @Test
    public void testWithVolume()
        throws Exception {
        Assert.assertTrue( new SeriesBuilder().withContext( VOLUME )
                                              .build()
                                          .getContext() != null );
  }

  @Test
  public void testWithDatapoints()
    throws Exception {
    final List<Datapoint> points = new ArrayList<>();
    points.add( new DatapointBuilder().withX( 12345.0 )
                                      .withY( 67890.0 )
                                      .build() );

    Assert.assertTrue( new SeriesBuilder().withDatapoints( points )
                                          .build()
                                          .getDatapoints() != null );
  }

  @Test
  public void testBuild()
    throws Exception {
    final List<Series> series = new ArrayList<>();

    final long timestamp = ( System.currentTimeMillis() / 1000 );
    final Series series1 =
      new SeriesBuilder().withContext( VOLUME )
                         .withDatapoint( new DatapointBuilder().withX( 25.0 )
                                                               .withY( (double)timestamp )
                                                               .build() )
                         .build();
    final Series series2 =
      new SeriesBuilder().withContext( VOLUME1 )
                         .withDatapoint( new DatapointBuilder().withX( 55.0 )
                                                               .withY( (double)timestamp )
                                                               .build() )
                         .withDatapoint( new DatapointBuilder().withX( 15.0 )
                                                               .withY( (double)timestamp )
                                                               .build() )
                         .build();
    System.out.println( series1 );
    Assert.assertTrue( series1.getDatapoints() != null );
    Assert.assertTrue( series1.getContext() != null );

    series.add( series1 );

    Assert.assertTrue( series2.getDatapoints() != null );
    Assert.assertTrue( series2.getContext() != null );
    series.add( series2 );

    System.out.println( ObjectModelHelper.toJSON( series ) );
  }
}