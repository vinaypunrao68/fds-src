/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Datapoint;
import org.junit.Assert;
import org.junit.Test;

public class DatapointBuilderTest {
  private static final Double x = 567890.0;
  private static final Double y = 1412126335.0;
  @Test
  public void testWithX()
    throws Exception {
    Assert.assertSame( new DatapointBuilder().withX( x )
                                             .build()
                                             .getX(),
                       x );
  }

  @Test
  public void testWithY()
    throws Exception {
    Assert.assertSame( new DatapointBuilder().withY( y )
                                             .build()
                                             .getY(),
                       y );
  }

  @Test
  public void testBuild()
    throws Exception {
    final Datapoint datapoint = new DatapointBuilder().withX( x )
                                                      .withY( y )
                                                      .build();
    Assert.assertSame( datapoint.getX(), x );
    Assert.assertSame( datapoint.getY(), y );
  }
}