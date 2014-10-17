/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Series;
import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.Usage;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.util.SizeUnit;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class StatisticsBuilderTest {
  private static final String EXPECTED_NAME =
    "TestVolume %s";
  private static final long EXPECTED_LIMIT = 300;
  private static final long EXPECTED_SLA = 0;
  private static final int EXPECTED_PRIORITY = 10;

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

  final static List<Series> series = new ArrayList<>();

  static {
    series.add( new SeriesBuilder().withContext( volume )
                                   .withDatapoint( new DatapointBuilder().withX( 55L )
                                                                         .withY( 123456789L )
                                                                         .build() )
                                   .withDatapoint( new DatapointBuilder().withX( 15L )
                                                                         .withY( 23456789L )
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
                                                            .withDatapoint( new DatapointBuilder().withX( 55L )
                                                                                                  .withY( 123456789L )
                                                                                                  .build() )
                                                            .build() )
                             .addSeries( new SeriesBuilder().withContext( volume )
                                                            .withDatapoint( new DatapointBuilder().withX( 15L )
                                                                                                  .withY( 23456789L )
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