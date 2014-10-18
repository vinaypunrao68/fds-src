/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
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
  private static final long EXPECTED_LIMIT = 300L;
  private static final long EXPECTED_SLA = 0L;
  private static final int EXPECTED_PRIORITY = 10;

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
        .withSla( EXPECTED_SLA )
        .build();

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
    final Volume volume = new VolumeBuilder().withId( "ID Only" )
                                             .build();
    Assert.assertSame( volume.getId(), "ID Only" );
  }

  private static final String NATE_JSON =
    "{\"priority\":10,\"sla\":0,\"limit\":300,\"snapshotPolicies\":[],\"name\":\"thirdtry\",\"data_connector\":{\"type\":\"Object\",\"api\":\"S3, Swift\"}}";

  @Test
  public void nate() {
    final Volume volume = ObjectModelHelper.toObject( NATE_JSON, Volume.class );
    System.out.println( volume );
  }

  @Test
  public void slaRangeFailTest() {
    try {
      new VolumeBuilder().withSla( -1 )   // minimum IOPS
        .build();

      Assert.fail( "Expected to be out of range lower limit for SLA!" );
    } catch( Exception ignore ) {

    }

    try {
      new VolumeBuilder().withLimit( 3001 )   // maximum IOPS
        .build();
      Assert.fail( "Expected to be out of range upper limit for SLA!" );
    } catch( Exception ignore ) {
    }
  }

  @Test
  public void priorityRangeFailTest() {
    try {
      new VolumeBuilder().withPriority( 0 )
                         .build();
      Assert.fail( "Expected to be out of range lower limit for Priority!" );
    } catch( Exception ignore ) {
    }

    try {
      new VolumeBuilder().withPriority( 11 )
                         .build();
      Assert.fail( "Expected to be out of range upper limit for Priority!" );
    } catch( Exception ignore ) {
    }
  }
}