/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.util.SizeUnit;
import org.junit.Assert;
import org.junit.Test;

public class ConnectorAttributesBuilderTest {
  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GB;
  private static final long EXPECTED_SIZE = 100;

  @Test
  public void test()
  {
    final ConnectorAttributes attrs =
      new ConnectorAttributesBuilder().withSize( EXPECTED_SIZE )
                                      .withUnit( EXPECTED_UNITS )
                                      .build();

    Assert.assertEquals( attrs.getSize(), EXPECTED_SIZE );
    Assert.assertEquals( attrs.getUnit(), EXPECTED_UNITS );
  }

}