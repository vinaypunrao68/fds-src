/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Usage;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.util.SizeUnit;
import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class SeriesBuilderTest {
  private static final String EXPECTED_NAME =
    "TestVolume %s";
  private static final long EXPECTED_LIMIT = 1024L;
  private static final long EXPECTED_SLA = 256L;
  private static final int EXPECTED_PRIORITY = 30;

  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GB;
  private static final long EXPECTED_SIZE = 100;
  private static final String EXPECTED_ID = "91237403";

  private static final Usage usage =
    new UsageBuilder().withSize( String.valueOf( EXPECTED_SIZE ) )
                      .withUnit( EXPECTED_UNITS )
                      .build();

  private static final Volume volume =
    new VolumeBuilder().withId( EXPECTED_ID )
                       .withName( String.format( EXPECTED_NAME, "1" ) )
                       .withCurrent_usage( usage )
                       .withPriority( EXPECTED_PRIORITY )
                       .withLimit( EXPECTED_LIMIT )
                       .withSla( EXPECTED_SLA )
                       .build();
  private static final Volume volume1 =
    new VolumeBuilder().withId( EXPECTED_ID )
                       .withName( String.format( EXPECTED_NAME, "2" ) )
                       .withCurrent_usage( usage )
                       .withPriority( EXPECTED_PRIORITY )
                       .withLimit( EXPECTED_LIMIT )
                       .withSla( EXPECTED_SLA )
                       .build();

  @Test
  public void testWithVolume()
    throws Exception {
    Assert.assertTrue( new SeriesBuilder().withContext( volume )
                                          .build()
                                          .getContext() != null );
  }

  @Test
  public void testWithDatapoints()
    throws Exception {
    final List<Datapoint> points = new ArrayList<>( );
    points.add( new DatapointBuilder().withX( 12345L )
                                      .withY( 67890L )
                                      .build() );

    Assert.assertTrue( new SeriesBuilder().withDatapoints( points )
                                          .build()
                                          .getDatapoints() != null );
  }

  @Test
  public void testBuild()
    throws Exception {
    final List<Series> series = new ArrayList<>( );

    final long timestamp = ( System.currentTimeMillis() / 1000 );
    final Series series1 =
      new SeriesBuilder().withContext( volume )
                         .withDatapoint( new DatapointBuilder().withX( 25L )
                                                               .withY( timestamp )
                                                               .build() )
                         .build();
    final Series series2 =
      new SeriesBuilder().withContext( volume1 )
                         .withDatapoint( new DatapointBuilder().withX( 55L )
                                                               .withY( timestamp )
                                                               .build() )
                         .withDatapoint( new DatapointBuilder().withX( 15L )
                                                               .withY( timestamp )
                                                               .build( ) ).build();
    Assert.assertTrue( series1.getDatapoints() != null );
    Assert.assertTrue( series1.getContext() != null );

    series.add( series1 );
    series.add( series2 );

    System.out.println( ObjectModelHelper.toJSON( series ) );
  }
}