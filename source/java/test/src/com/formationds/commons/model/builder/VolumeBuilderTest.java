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
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.ConnectorType;
import com.formationds.util.SizeUnit;
import org.junit.Assert;
import org.junit.Test;

public class VolumeBuilderTest {
  private static final String EXPECTED_APIS = "S3, Swift";
  private static final String EXPECTED_NAME =
    "Volume Name -- " + ( System.currentTimeMillis() / 20000 );
  private static final long EXPECTED_LIMIT = 1024L;
  private static final long EXPECTED_SLA = 256L;
  private static final int EXPECTED_PRIORITY = 30;

  private static final SizeUnit EXPECTED_UNITS = SizeUnit.GB;
  private static final long EXPECTED_SIZE = 100;

  private static final ConnectorType EXPECTED_TYPE = ConnectorType.OBJECT;

  @Test
  public void test() {

//      final ConnectorAttributes attrs =
//        new ConnectorAttributesBuilder().withSize( EXPECTED_SIZE )
//                                        .withUnit( EXPECTED_UNITS )
//                                        .build();
      final Connector connector =
        new ConnectorBuilder()// .withAttributes( attrs )
                              .withType( EXPECTED_TYPE )
                              .withApi( "S3, Swift" )
                              .build();

//    final Usage usage = new UsageBuilder().withSize( String.valueOf( EXPECTED_SIZE ) )
//                                          .withUnit( EXPECTED_UNITS )
//                                          .build();

    final Volume volume =
      new VolumeBuilder()
//                         .withCurrent_usage( usage )
                         .withData_connector( connector )
                         .withLimit( EXPECTED_LIMIT )
                         .withName( EXPECTED_NAME )
                         .withPriority( EXPECTED_PRIORITY )
                         .withSla( EXPECTED_SLA ).build();

//      System.out.println( ObjectModelHelper.toJSON( volume ) );

//      Assert.assertNotNull( volume.getCurrent_usage() );

      Assert.assertNotNull( volume.getData_connector() );
//      Assert.assertNotNull( connector.getAttributes() );
//      Assert.assertEquals( connector.getAttributes().getSize(), EXPECTED_SIZE );
//      Assert.assertEquals( connector.getAttributes().getUnit(), EXPECTED_UNITS );
      Assert.assertEquals( connector.getType(), EXPECTED_TYPE );
    Assert.assertEquals( connector.getApi(), EXPECTED_APIS );

      Assert.assertEquals( volume.getLimit(), EXPECTED_LIMIT );
      Assert.assertEquals( volume.getName(), EXPECTED_NAME );
      Assert.assertEquals( volume.getPriority(), EXPECTED_PRIORITY );
      Assert.assertEquals( volume.getSla(), EXPECTED_SLA );
  }

  // NullPointer when priority is not supplied
  @Test
  public void volumeId() {
    final Volume volume = new VolumeBuilder().withId( "ID Only" ).build();
    Assert.assertSame( volume.getId(), "ID Only" );
  }

  private static final String NATE_JSON =
    "{\"priority\":10,\"sla\":0,\"limit\":300,\"snapshotPolicies\":[],\"name\":\"thirdtry\",\"data_connector\":{\"type\":\"Object\",\"api\":\"S3, Swift\"}}";

  @Test
  public void nate()
  {
    final Volume volume = ObjectModelHelper.toObject( NATE_JSON, Volume.class );
    System.out.println( volume );
  }
}