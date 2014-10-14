/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.entity.MetricQuery;
import com.formationds.commons.model.entity.builder.MetricQueryBuilder;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.model.type.ResultType;
import org.junit.Assert;
import org.junit.Test;

import java.util.Calendar;

public class MetricQueryBuilderTest {
  private static final Calendar cal2DaysAgo = Calendar.getInstance();

  static {
    cal2DaysAgo.add( Calendar.DAY_OF_MONTH, -2 );
  }

  private static final Calendar cal = Calendar.getInstance();

  @Test
  public void withRangeTest() {
    final MetricQuery metricQuery =
      new MetricQueryBuilder().withRange( new DateRangeBuilder().withStart( cal2DaysAgo.getTime() )
                                                                .withEnd( cal.getTime() )
                                                                .build() )
                              .build();
    Assert.assertTrue( metricQuery.getRange() != null );
  }

  @Test
  public void withSeriesTypeTest() {
    final MetricQuery metricQuery =
      new MetricQueryBuilder().withSeriesType( Metrics.PUTS )
                              .build();
    Assert.assertTrue( metricQuery.getSeriesType() != null );

    final MetricQuery metricQuery1 =
      new MetricQueryBuilder().withSeriesType( "GETS" )
                              .build();
    Assert.assertTrue( metricQuery1.getSeriesType() != null );
  }

  @Test
  public void withContextTest() {
    final MetricQuery metricQuery =
      new MetricQueryBuilder().withContext( new VolumeBuilder().withName( "TestVolume" )
                                                               .build() )
                              .withContext( new VolumeBuilder().withName( "TestVolume 1" )
                                                               .build() )
                              .build();
    Assert.assertTrue( metricQuery.getContexts() != null );
    Assert.assertTrue( metricQuery.getContexts()
                                  .size() == 2 );
  }

  @Test
  public void withResultTypeTest() {
    final MetricQuery metricQuery =
      new MetricQueryBuilder().withResultType( ResultType.SUM )
                              .build();
    Assert.assertTrue( metricQuery.getResultType() != null );
    final MetricQuery metricQuery1 =
      new MetricQueryBuilder().withResultType( "SUM" )
                              .build();
    Assert.assertTrue( metricQuery1.getResultType() != null );
  }
}