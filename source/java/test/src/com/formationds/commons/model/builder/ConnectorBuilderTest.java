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

import com.formationds.commons.model.Connector;
import com.formationds.commons.model.ConnectorAttributes;
import com.formationds.commons.model.type.DataConnectorType;
import com.formationds.util.SizeUnit;
import org.junit.Assert;
import org.junit.Test;

public class ConnectorBuilderTest {
  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GB;
  private static final long EXPECTED_SIZE = 100;

  private static final DataConnectorType EXPECTED_TYPE =
    DataConnectorType.OBJECT;

  @Test
  public void test()
  {
    final ConnectorAttributes attrs =
      new ConnectorAttributesBuilder().withSize( EXPECTED_SIZE )
                                      .withUnit( EXPECTED_UNITS )
                                      .build();
    final Connector connector =
      new ConnectorBuilder().withAttributes( attrs )
                            .withType( EXPECTED_TYPE )
                            .build();
    Assert.assertNotNull( connector.getAttributes() );
    Assert.assertEquals( connector.getAttributes().getSize(), EXPECTED_SIZE );
    Assert.assertEquals( connector.getAttributes().getUnit(), EXPECTED_UNITS );
    Assert.assertEquals( connector.getType(), EXPECTED_TYPE );
  }

}