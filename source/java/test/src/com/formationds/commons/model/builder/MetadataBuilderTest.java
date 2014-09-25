/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.builder;

import com.formationds.commons.model.Metadata;
import org.junit.Assert;
import org.junit.Test;

public class MetadataBuilderTest {
  private static final String EXPECTED_KEY = "METADATA_KEY";
  private static final String EXPECTED_VALUE = "METADATA_VALUE";
  private static final long EXPECTED_TIMESTAMP =
    System.currentTimeMillis() / 1000;
  private static final String EXPECTED_VOLUME_NAME = "VOLUME_NAME";

  @Test
  public void test() {
    final Metadata metadata =
      new MetadataBuilder().withKey( EXPECTED_KEY )
                           .withTimestamp( EXPECTED_TIMESTAMP )
                           .withValue( EXPECTED_VALUE )
                           .withVolume( EXPECTED_VOLUME_NAME )
                           .build();

    Assert.assertEquals( metadata.getKey(), EXPECTED_KEY );
    Assert.assertEquals( metadata.getValue(), EXPECTED_VALUE );
    Assert.assertEquals( metadata.getVolume(), EXPECTED_VOLUME_NAME );
    Assert.assertEquals( metadata.getTimestamp(), EXPECTED_TIMESTAMP );
  }

}