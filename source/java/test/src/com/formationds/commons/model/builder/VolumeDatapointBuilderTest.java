/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.om.repository.query.builder.VolumeDatapointBuilder;
import org.junit.Assert;
import org.junit.Test;

public class VolumeDatapointBuilderTest {
  private static final Long timestamp = 1412126335L;
  private static final String volumeName = "volume-name";
  private static final String key = "datapoint-key";
  private static final Double value = 67.7;

  @Test
  public void testWithTimestamp()
    throws Exception {
    Assert.assertEquals( new VolumeDatapointBuilder().withTimestamp( timestamp )
                                                     .build()
                                                     .getTimestamp(),
                         timestamp );
  }

  @Test
  public void testWithVolumeName()
    throws Exception {
    Assert.assertEquals( new VolumeDatapointBuilder().withVolumeName( volumeName )
                                                     .build()
                                                     .getVolumeName(),
                         volumeName );
  }

  @Test
  public void testWithKey()
    throws Exception {
    Assert.assertEquals( new VolumeDatapointBuilder().withKey( key )
                                                     .build()
                                                     .getKey(),
                         key );
  }

  @Test
  public void testWithValue()
    throws Exception {
    Assert.assertEquals( new VolumeDatapointBuilder().withValue( value )
                                                     .build()
                                                     .getValue(),
                         value );
  }

  @Test
  public void testBuild()
    throws Exception {
    final VolumeDatapoint datapoint =
      new VolumeDatapointBuilder().withKey( key )
                                  .withVolumeName( volumeName )
                                  .withValue( value )
                                  .withTimestamp( timestamp )
                                  .build();

    Assert.assertSame( datapoint.getKey(), key );
    Assert.assertSame( datapoint.getValue(), value );
    Assert.assertSame( datapoint.getVolumeName(), volumeName );
    Assert.assertSame( datapoint.getTimestamp(), timestamp );
  }
}